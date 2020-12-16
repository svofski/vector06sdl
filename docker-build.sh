docker build -t svo/v06x .
docker run -it -v$(pwd):/builder --rm --name "vector_builder" svo/v06x:latest /usr/bin/make clean deb windows
