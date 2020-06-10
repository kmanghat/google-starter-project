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
using foodvendor::VendorList;
using foodvendor::FoodvendorService;
using foodvendor::FoodsupplierService;
using foodvendor::Ingredient;
using foodvendor::IngredientRequest;

// Logic and data behind the server's behavior.
class FoodsupplierServiceImpl final : public FoodsupplierService::Service {
  
    Status FindVendors(ServerContext* context, const IngredientRequest* request,
                  VendorList* vendorList) override {
              
        std::string vendorName = "";
        std::ifstream datafile;
        datafile.open("database/data.txt");
        
        if(!datafile.is_open())std::cout << "Error reading data\n";
        else std::cout << "Looking for vendors selling " << request->name() << "\n";
        
        while(datafile >> vendorName){
            char semi;
            while(datafile >> semi && semi != '.'){
                std::string ingredientName;
                datafile >> ingredientName;
                datafile.ignore();

                if(ingredientName == request->name()){
                    Vendor *vendor = vendorList->add_vendors();
                    vendor->set_name(vendorName);
                }
            }
        }
        return Status::OK;
    }
};

void RunServer() {
  std::string address = "0.0.0.0";
  std::string port = "50053";
  std::string server_address = address + ":" + port;
  FoodsupplierServiceImpl service;

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
