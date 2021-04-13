FROM debian
COPY 727_uya.cpp .
COPY create_files.sh .
RUN chmod ugo+x create_files.sh
RUN apt-get update
RUN apt-get -y install g++
RUN g++ -o 727_uya 727_uya.cpp -pthread
RUN ./create_files.sh
CMD ./727_uya
