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

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using foodvendor::VendorRequest;
using foodvendor::Vendor;
using foodvendor::FoodvendorService;
using foodvendor::Ingredient;


// Logic and data behind the server's behavior.
class FoodVendorServiceImpl final : public FoodvendorService::Service {
  
    Status GetIngredientInfo(ServerContext* context, const VendorRequest* vendorRequest,
                  Ingredient* ingredient) override {
    
        std::string ingredientName = "";
        std::ifstream datafile;
        datafile.open("database/"+vendorRequest->name()+".txt");
        if(!datafile.is_open())std::cout << "Error reading data\n";
        else std::cout << "Looking for " << vendorRequest->ingredientname() << " sold by " << vendorRequest->name() << "\n";
        
        while(datafile >> ingredientName){
            double priceInDollars;
            int inventory;
            datafile >> priceInDollars;
            datafile >> inventory;
            datafile.ignore();
            if(ingredientName == vendorRequest->ingredientname()){
            
                ingredient->set_name(ingredientName);
                ingredient->set_price(priceInDollars);
                ingredient->set_quantity(inventory);
            }
        }
        
        return Status::OK;
    }
  
};

void RunServer() {
    std::string address = "0.0.0.0";
    std::string port = "50051";
    std::string server_address = address + ":" + port;
    FoodVendorServiceImpl service;

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
