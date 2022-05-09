FROM centos:centos8

RUN cd /etc/yum.repos.d/
RUN sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-*
RUN sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*

RUN dnf groupinstall "Development Tools" -y
RUN dnf install gcc* make cmake* rpm-build wget tar git mpich* vim pkgconfig gedit -y

RUN dnf install openmpi* -y
RUN dnf install fftw-* -y

RUN wget https://download.open-mpi.org/release/open-mpi/v4.1/openmpi-4.1.3.tar.gz
RUN tar -xvzf openmpi-4.1.3.tar.gz && cd openmpi-4.1.3 && ./configure --enable-mpi-cxx && make && make install

RUN git clone https://github.com/demorest/vdifio.git && cd vdifio && aclocal -I m4 && libtoolize --copy --force && autoconf && autoheader && automake -a -c && ./configure --enable-python && make && make install

RUN git clone https://gitlab.com/sklandrausis1/kana.git && cd kana && cd correlator-kana && mkdir build && cd build && cmake .. && make && make install

RUN dnf install libjpeg-* -y

RUN dnf install python3-devel -y

RUN pip3 install pillow numpy matplotlib
