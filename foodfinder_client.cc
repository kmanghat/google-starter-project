/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "opencensus/trace/sampler.h"
#include "opencensus/trace/span.h"
#include <grpcpp/grpcpp.h>
#include "opencensus/exporters/trace/zipkin/zipkin_exporter.h"

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
using foodvendor::FoodvendorService;
using foodvendor::FoodsupplierService;
using foodvendor::Ingredient;
using foodvendor::IngredientRequest;
using foodvendor::FoodfinderService;
using foodvendor::IngredientRequest;
using foodvendor::IngredientAvailability;
using foodvendor::VendorList;


class FoodfinderClient {
    public:
    FoodfinderClient(std::shared_ptr<Channel> channel)
      : stub_(FoodfinderService::NewStub(channel)) {}

    // Assembles the client's payload, sends it and presents the response back
    // from the server.
    IngredientAvailability GetAvailableIngredients(const std::string& user) {
        // Data we are sending to the server.
        IngredientRequest request;
        request.set_name(user);

        // Container for the data we expect from the server.
        IngredientAvailability ingredientAvailability;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // The actual RPC.
        Status status = stub_->FindIngredient(&context, request, &ingredientAvailability);

        // Act upon its status.
        if (status.ok()) {
            return ingredientAvailability;
        } else {
            std::cout << status.error_code() << ": " << status.error_message()
                    << std::endl;
            return {};
        }
    }

 private:
    std::unique_ptr<FoodfinderService::Stub> stub_;
};

int main(int argc, char** argv) {
    // Instantiate the client. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint specified by
    // the argument "--target=" which is the only expected argument.
    // We indicate that the channel isn't authenticated (use of
    // InsecureChannelCredentials()).
    const absl::string_view endpoint = "http://localhost:9411/api/v2/spans";
      
      
    std::string target_str;
    std::string arg_str("--target");
    if (argc > 1) {
        std::string arg_val = argv[1];
        size_t start_pos = arg_val.find(arg_str);
        if (start_pos != std::string::npos) {
            start_pos += arg_str.size();
            if (arg_val[start_pos] == '=') {
                target_str = arg_val.substr(start_pos + 1);
            } else {
                std::cout << "The only correct argument syntax is --target=" << std::endl;
                return 0;
            }
        } else {
            std::cout << "The only acceptable argument is --target=" << std::endl;
            return 0;
        }
    } else {
        target_str = "localhost:50054";
    }
    
    FoodfinderClient finderClient(grpc::CreateChannel(
      target_str, grpc::InsecureChannelCredentials()));
    std::cout << "Welcome to the ingredient finder application\n\n";
    
    std::string ingredient;
    std::cout << "Enter ingredient or ctrl+c to quit: ";
    while(std::cin >> ingredient){
        IngredientAvailability vendorList = finderClient.GetAvailableIngredients(ingredient);
        
        if(vendorList.vendoringredientinfo_size() == 0) std::cout << "No vendors found :(\n";
        for(int i = 0; i < vendorList.vendoringredientinfo_size(); i++)
        {
            std::cout << vendorList.vendoringredientinfo(i).name() << " sells " << vendorList.vendoringredientinfo(i).ingredient().name() 
            << " at " << vendorList.vendoringredientinfo(i).ingredient().price() << " dollars and has " << 
            vendorList.vendoringredientinfo(i).ingredient().quantity() << " units " << "\n";
        }
        std::cout << "\n";
        std::cout << "Enter ingredient or ctrl+c to quit: ";
    }

    return 0;
}
