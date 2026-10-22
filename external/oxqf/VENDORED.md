# Vendored OXQF subset

Source: C:\Users\yy_1070\project\OXQF at 7d8a554, MIT (LICENSE).

The C++ model, codec, conversion and generated Unicode table sources are unchanged. This copy is used as two native static libraries, OXQF::core and OXQF::convert.

The local CMakeLists.txt is a minimal application adapter. Upstream Node release/fixture-generation scripts and the Node CLI wrapper test are omitted; their test registration is removed. Checked-in C++ tables and fixtures are retained, so application builds and tests require no Node toolchain.

The original README/specification and C++ test sources are retained for reference. Build this dependency through the application root, not the original standalone release workflow. The user's original OXQF repository is untouched.
