workspace(name = "rockssnmp")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")


http_archive(
    name = "toolchain",
    urls = [
        # The file: URL is useful for testing the build, but is not generally necessary since Bazel handles caching
        # external dependencies.
        # TODO(james): Remove this URL when the Bazel build is stable.
        # "file:///home/james/git/toolchain-master.tgz",
        "https://github.com/stardog-union/toolchain/archive/master.zip",
    ],
    strip_prefix = "toolchain-master",
    sha256 = "d0740cacb99833911baba82041bb4429f9d3182522fe0fd4c131335ac8343891",
)
