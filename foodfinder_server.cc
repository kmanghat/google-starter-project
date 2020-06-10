#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <fstream>
#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "proto/foodvendor.grpc.pb.h"
#else
#include "proto/foodvendor.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using foodvendor::VendorRequest;
using foodvendor::Vendor;
using foodvendor::VendorList;
using foodvendor::FoodvendorService;
using foodvendor::FoodsupplierService;
using foodvendor::Ingredient;
using foodvendor::IngredientRequest;
using foodvendor::FoodfinderService;
using foodvendor::IngredientRequest;
using foodvendor::IngredientAvailability;
using foodvendor::VendorIngredientInfo;


class FoodsupplierClient {
    public:
    FoodsupplierClient(std::shared_ptr<Channel> channel)
      : stub_(FoodsupplierService::NewStub(channel)) {}

    // Assembles the client's payload, sends it and presents the response back
    // from the server.
    std::vector<Vendor> FindVendors(const std::string& user) {
        // Data we are sending to the server.
        IngredientRequest request;
        request.set_name(user);

        // Container for the data we expect from the server.
        VendorList reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // The actual RPC.
        Status status = stub_->FindVendors(&context, request, &reply);
        std::cout << "Supplier client\n";
        // Act upon its status.
        if (status.ok()) {
            std::vector<Vendor> vendorList;
            for(int i = 0; i < reply.vendors_size(); i++){
                Vendor vendor;
                vendor.set_name(reply.vendors(i).name());
                vendorList.push_back(vendor);
            }
            return vendorList;
        } else {
          std::cout << status.error_code() << ": " << status.error_message()
                    << std::endl;
          return {};
        }
    }

    private:
    std::unique_ptr<FoodsupplierService::Stub> stub_;
};


class FoodvendorClient {
    public:
    FoodvendorClient(std::shared_ptr<Channel> channel)
      : stub_(FoodvendorService::NewStub(channel)) {}

        // Assembles the client's payload, sends it and presents the response back
        // from the server.
        Ingredient FindIngredient(const std::string& user, const std::string& ingredientName) {
            // Data we are sending to the server.
            VendorRequest request;
            request.set_name(user);
            request.set_ingredientname(ingredientName);

            // Container for the data we expect from the server.
            Ingredient ingredient;

            // Context for the client. It could be used to convey extra information to
            // the server and/or tweak certain RPC behaviors.
            ClientContext context;

            // The actual RPC.
            Status status = stub_->GetIngredientInfo(&context, request, &ingredient);

            // Act upon its status.
            if (status.ok()) {
                return ingredient;
            } else {
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
              return {};
            }
        }

    private:
    std::unique_ptr<FoodvendorService::Stub> stub_;
};

class FoodfinderServiceImpl final : public FoodfinderService::Service {
    Status FindIngredient(ServerContext* context, const IngredientRequest* request,
                  IngredientAvailability* ingredientAvailability) override {        
        
        //For each vendor query foodvendor for price and available quantity of that ingredient
        std::cout << "Server starting request for " << request->name() << "\n";
        //Query the supplier client for a list of vendors selling the ingredient 
        FoodsupplierClient supplierClient(grpc::CreateChannel(
          "localhost:50053", grpc::InsecureChannelCredentials()));
        std::vector<Vendor> vendorList = supplierClient.FindVendors(request->name());

        for(auto vendor: vendorList){
            
            VendorIngredientInfo* vendorIngredientInfo = ingredientAvailability->add_vendoringredientinfo();
            vendorIngredientInfo->set_name(vendor.name());
            
            FoodvendorClient vendorClient(grpc::CreateChannel(
             "localhost:50051", grpc::InsecureChannelCredentials()));
            Ingredient ingredient = vendorClient.FindIngredient(vendor.name(),request->name());
            vendorIngredientInfo->mutable_ingredient()->CopyFrom(ingredient);
        }
        
        return Status::OK;
    }
};

void RunServer() {
    std::string address = "0.0.0.0";
    std::string port = "50054";
    std::string server_address = address + ":" + port;
    FoodfinderServiceImpl service;

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}
