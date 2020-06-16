#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include <grpcpp/opencensus.h>

#ifdef BAZEL_BUILD
#include "opencensus/exporters/trace/zipkin/zipkin_exporter.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "opencensus/exporters/trace/zipkin/zipkin_exporter.h"
#include "opencensus/exporters/stats/stackdriver/stackdriver_exporter.h"
#include "opencensus/trace/sampler.h"
#include "opencensus/trace/span.h"
#include "opencensus/stats/stats.h"
#include "proto/foodvendor.grpc.pb.h"
#else
#include "opencensus/exporters/trace/zipkin/zipkin_exporter.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "opencensus/exporters/trace/zipkin/zipkin_exporter.h"
#include "opencensus/exporters/stats/stackdriver/stackdriver_exporter.h"
#include "opencensus/trace/sampler.h"
#include "opencensus/trace/span.h"
#include "opencensus/stats/stats.h"
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


const std::string kGeneralErrorString = "ERROR";
const int kServerTimeout = 85;

// For metrics
ABSL_CONST_INIT const absl::string_view kRPCErrorMeasureName = "rpc_error_count";
ABSL_CONST_INIT const absl::string_view kRPCCountMeasureName = "rpc_count";
ABSL_CONST_INIT const absl::string_view kRPCLatencyMeasureName = "rpc_latency";


// For metrics
opencensus::stats::MeasureInt64 RPCErrorCountMeasure() {
  static const auto measure =
      opencensus::stats::MeasureInt64::Register(
          kRPCErrorMeasureName, "Number of RPC errors encountered.", "By");
  return measure;
}

opencensus::stats::MeasureInt64 RPCCountMeasure() {
  static const auto measure =
      opencensus::stats::MeasureInt64::Register(
          kRPCCountMeasureName, "Number of RPC calls made.", "By");
  return measure;
}

opencensus::stats::MeasureDouble RPCLatencyMeasure() {
  static const auto measure =
      opencensus::stats::MeasureDouble::Register(
          kRPCLatencyMeasureName, "Latency of RPC calls made.", "ms");
  return measure;
}

opencensus::tags::TagKey MethodKey() {
  static const opencensus::tags::TagKey key =
      opencensus::tags::TagKey::Register("method");
  return key;
}

void RegisterViews() {
    RPCErrorCountMeasure();
    opencensus::stats::ViewDescriptor()
        .set_name("FoodService/RPCErrorCount")
        .set_description("Number of RPC errors")
        .set_measure(kRPCErrorMeasureName)
        .set_aggregation(opencensus::stats::Aggregation::Count())
        .add_column(MethodKey())
        .RegisterForExport();

    RPCCountMeasure();
    opencensus::stats::ViewDescriptor()
        .set_name("FoodService/RPCCount")
        .set_description("Number of RPC calls")
        .set_measure(kRPCCountMeasureName)
        .set_aggregation(opencensus::stats::Aggregation::Count())
        .add_column(MethodKey())
        .RegisterForExport();

    RPCLatencyMeasure();
    opencensus::stats::ViewDescriptor()
        .set_name("FoodService/RPCLatency")
        .set_description("Latency of RPC calls")
        .set_measure(kRPCCountMeasureName)
        .set_aggregation(opencensus::stats::Aggregation::Distribution(
          opencensus::stats::BucketBoundaries::Explicit(
              {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100})))
        .add_column(MethodKey())
        .RegisterForExport();
}


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
        
           
        static opencensus::trace::AlwaysSampler sampler;
        
        // Begin FoodFinder span
        opencensus::trace::Span finder_span = opencensus::trace::Span::StartSpan(
            "FoodFinder", /* parent = */ nullptr, {&sampler});
        finder_span.AddAnnotation("Requested ingredient: " + request->name());
    
    
        // Begin FoodSupplier span
        opencensus::trace::Span supplier_span = opencensus::trace::Span::StartSpan(
        "FoodSupplier", &finder_span, {&sampler});
        
        
        // For metrics
        grpc::RegisterOpenCensusPlugin();
        grpc::RegisterOpenCensusViewsForExport();


        RegisterExporters();
        
        std::vector<Vendor> vendorList = supplierClient.FindVendors(request->name());
        supplier_span.AddAnnotation(std::to_string(vendorList.size()) + " vendors found");
        supplier_span.End();
        
        // Begin FoodVendor span
        opencensus::trace::Span vendor_span = opencensus::trace::Span::StartSpan(
            "FoodVendor", &finder_span, {&sampler});
        for(auto vendor: vendorList){
            
            VendorIngredientInfo* vendorIngredientInfo = ingredientAvailability->add_vendoringredientinfo();
            vendorIngredientInfo->set_name(vendor.name());
            const std::string span_name = "FoodVendor - " + vendor.name();
        opencensus::trace::Span curr_vendor_span = opencensus::trace::Span::StartSpan(
            span_name, &vendor_span, {&sampler});
            
            FoodvendorClient vendorClient(grpc::CreateChannel(
             "localhost:50051", grpc::InsecureChannelCredentials()));
            Ingredient ingredient = vendorClient.FindIngredient(vendor.name(),request->name());
            vendorIngredientInfo->mutable_ingredient()->CopyFrom(ingredient);
            
            curr_vendor_span.End();
        }
        vendor_span.End();
        finder_span.End();
        return Status::OK;
    }
    
    void RegisterExporters() {
        const absl::string_view endpoint_ = "http://localhost:9411/api/v2/spans";
        // Zipkin
        opencensus::exporters::trace::ZipkinExporterOptions options = opencensus::exporters::trace::ZipkinExporterOptions(endpoint_);
        options.service_name = "FoodService";
        opencensus::exporters::trace::ZipkinExporter::Register(options);

        // StackDriver
        const char* project_id = getenv("STACKDRIVER_PROJECT_ID");
        if (project_id == nullptr) {
            std::cerr << "The STACKDRIVER_PROJECT_ID environment variable is not set: "
                        "not exporting to Stackdriver.\n";
        }
        else {
            opencensus::exporters::stats::StackdriverOptions stats_opts;
            stats_opts.project_id = project_id;
            opencensus::exporters::stats::StackdriverExporter::Register(
                std::move(stats_opts));
        }
    }
};

void RunServer() {
    std::string address = "0.0.0.0";
    std::string port = "50054";
    std::string server_address = address + ":" + port;
    FoodfinderServiceImpl service;

    RegisterViews();
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
