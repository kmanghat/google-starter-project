Set up servers:

bazel run :foodvendor_server

bazel run :foodfinder_server

bazel run :foodsupplier_server


How to use:

bazel run :foodfinder_client

At the prompt type a single ingredient you want to find

Query for ingredients {flour,chocolate etc} (Vendors and ingredients can be seen in database/data.txt and vendor files)
