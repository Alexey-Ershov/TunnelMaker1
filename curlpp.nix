{ stdenv, fetchurl, cmake, unzip, curl }:

stdenv.mkDerivation rec {
    version = "v0.8.1";
    name = "curlpp-${version}";
    
    src = fetchurl {
      url = "https://github.com/jpbarrette/curlpp/archive/${version}.zip";
      sha256 = "1vnccibmnprwp015xz16zpcg0ksrk795i9x6p9v10panxqxr5fv7";
    };

    nativeBuildInputs = [ cmake unzip ];
    propagatedBuildInputs = [ curl ];
    enableParallelBuilding = true;
}
