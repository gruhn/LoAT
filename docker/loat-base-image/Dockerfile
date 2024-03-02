FROM voidlinux/voidlinux-musl:latest as base

ENV CFLAGS -march=x86-64 -O2
ENV CXXFLAGS $CFLAGS
RUN echo noextract=/etc/hosts > /etc/xbps.d/test.conf
RUN echo "repository=https://repo-default.voidlinux.org/current/musl" > /etc/xbps.d/00-repository-main.conf
RUN xbps-install -ySu xbps
RUN xbps-install -ySu
# Dependencies used by all (or most) subsequent build stages.
# Note that changing these invalidaes the cache for all subsequent
# build stages:
RUN xbps-install -yS cmake make wget gcc autoconf bash git

#######################################################

FROM base as z3
RUN xbps-install -yS python3-devel

RUN wget https://github.com/Z3Prover/z3/archive/refs/tags/z3-4.12.5.tar.gz
RUN tar xf z3-4.12.5.tar.gz
WORKDIR /z3-z3-4.12.5
RUN mkdir build
WORKDIR /z3-z3-4.12.5/build
RUN cmake -DZ3_BUILD_LIBZ3_SHARED=FALSE -DCMAKE_BUILD_TYPE=Release ..
RUN make -j4
RUN make install

#######################################################

FROM base as z3-binary
RUN xbps-install -yS python3-devel

RUN wget https://github.com/Z3Prover/z3/archive/refs/tags/z3-4.12.5.tar.gz
RUN tar xf z3-4.12.5.tar.gz
WORKDIR /z3-z3-4.12.5
RUN python scripts/mk_make.py --staticbin
WORKDIR /z3-z3-4.12.5/build
RUN make -j4
RUN make install

#######################################################

FROM base as ntl
RUN xbps-install -yS gmp-devel

RUN wget https://libntl.org/ntl-11.4.4.tar.gz
RUN tar xf ntl-11.4.4.tar.gz
WORKDIR /ntl-11.4.4/src
RUN ./configure
RUN make -j4
RUN make install

#######################################################

FROM base as cudd
RUN xbps-install -yS automake 

RUN git clone https://github.com/ivmai/cudd.git
WORKDIR /cudd
# make check fails when compiled with -DNDEBUG
RUN ./configure CFLAGS="$CFLAGS -fPIC" CXXFLAGS="$CXXFLAGS -fPIC"
RUN sed -i 's/aclocal-1.14/aclocal-1.16/g' Makefile
RUN sed -i 's/automake-1.14/automake-1.16/g' Makefile
RUN make -j4
RUN make -j4 check
RUN make install

#######################################################

FROM base as poly
RUN xbps-install -yS gmpxx-devel python-devel
# RUN xbps-install -yS 

RUN wget https://github.com/SRI-CSL/libpoly/archive/refs/tags/v0.1.13.tar.gz
RUN tar xf v0.1.13.tar.gz
WORKDIR /libpoly-0.1.13/build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
RUN make -j4
RUN make install

#######################################################

FROM base as cvc5
RUN xbps-install -yS cln-devel git libtool python3-devel python3-pip texinfo

RUN xbps-install -yS 
ENV VIRTUAL_ENV=/opt/venv
RUN python3 -m venv $VIRTUAL_ENV
ENV PATH="$VIRTUAL_ENV/bin:$PATH"
RUN pip install tomli pyparsing

COPY --from=poly /usr/local/lib/libpoly.a /usr/local/lib/
COPY --from=poly /usr/local/lib/libpolyxx.a /usr/local/lib/
COPY --from=poly /usr/local/include/poly /usr/local/include/poly

RUN git clone https://github.com/ffrohn/cvc5
WORKDIR cvc5
RUN git checkout cvc5-1.0.8-musl
RUN ./configure.sh --static --no-statistics --auto-download --poly --cln --gpl --no-docs
WORKDIR /cvc5/build
RUN make -j4
RUN make install

#######################################################

FROM base as ginac
RUN xbps-install -yS cln-devel python-devel

RUN git clone git://www.ginac.de/ginac.git
WORKDIR /ginac
RUN git checkout release_1-8-7
RUN mkdir build
WORKDIR /ginac/build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=false ..
RUN make -j4
RUN make install

#######################################################

FROM base as purrs
RUN xbps-install -yS automake cln-devel gmpxx-devel libtool readline-devel

COPY --from=ginac /usr/local/lib64/libginac.a /usr/local/lib64/libginac.a
COPY --from=ginac /usr/local/include/ginac /usr/local/include/ginac

COPY --from=ntl /usr/local/include/NTL /usr/local/include/NTL
COPY --from=ntl /usr/local/lib/libntl.a /usr/local/lib/libntl.a

RUN git clone https://github.com/aprove-developers/LoAT-purrs.git
WORKDIR /LoAT-purrs
RUN autoreconf --install
RUN automake
RUN ./configure
RUN make -j4
RUN make install

#######################################################

FROM base as swine
RUN xbps-install -yS boost-devel

COPY --from=z3 /usr/local/lib64/libz3.a /usr/local/lib64/
COPY --from=z3 /usr/local/include/ /usr/local/include/

RUN git clone https://github.com/ffrohn/swine-z3
WORKDIR /swine-z3/scripts
RUN ./configure_and_build.sh

#######################################################

FROM base as gmp
RUN xbps-install -yS lzip

RUN wget https://gmplib.org/download/gmp/gmp-6.2.1.tar.lz
RUN lzip -d gmp-6.2.1.tar.lz
RUN tar xf gmp-6.2.1.tar
WORKDIR /gmp-6.2.1
RUN ./configure ABI=64 CFLAGS="$CFLAGS -fPIC" CPPFLAGS="$CXXFLAGS -DPIC" --host=x86_64-pc-linux-gnu --enable-cxx --prefix /gmp/
RUN make -j4
RUN make -j4 check
RUN make install

#######################################################

FROM base as yices
RUN xbps-install -yS gmp-devel gperf

COPY --from=gmp /gmp /gmp

COPY --from=poly /usr/local/lib/libpoly.a /usr/local/lib/
COPY --from=poly /usr/local/include/poly /usr/local/include/poly

COPY --from=cudd /usr/local/lib/libcudd.a /usr/local/lib/libcudd.a
COPY --from=cudd /usr/local/include/cudd.h /usr/local/include/cudd.h

RUN git clone https://github.com/SRI-CSL/yices2.git
WORKDIR /yices2
RUN autoconf
RUN ./configure --enable-mcsat --with-pic-gmp=/gmp/lib/libgmp.a
RUN make -j4
RUN make -j4 static-lib
RUN make install

#######################################################

FROM base as antlr4
RUN xbps-install -yS git

RUN git clone https://github.com/antlr/antlr4.git
WORKDIR /antlr4
RUN git checkout 4.11.1
RUN mkdir /antlr4/runtime/Cpp/build
WORKDIR /antlr4/runtime/Cpp/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_LIBDIR="/usr/local/lib"
RUN make -j4
RUN make install

#######################################################

FROM base as faudes
RUN xbps-install -yS readline-devel
# RUN xbps-install -yS 

# The following step keeps breaking because libfaudes developers don't keep
# old archives around long after releasing new version. If it breaks go to
# https://fgdes.tf.fau.de/download.html and check for a recent version. 
# Then replace the value in the following env var:
ENV VERSION=2_32a

RUN wget https://fgdes.tf.fau.de/archive/libfaudes-${VERSION}.tar.gz
RUN tar xf libfaudes-${VERSION}.tar.gz
WORKDIR /libfaudes-${VERSION}
RUN sed -i 's/MAINOPTS += -std=gnu++98 -D_GLIBCXX_USE_CXX11_ABI=0/MAINOPTS += -std=c++11/g' Makefile
RUN FAUDES_LINKING=static make -j4
RUN cp /libfaudes-${VERSION}/libfaudes.a /usr/local/lib/
RUN cp -r /libfaudes-${VERSION}/include /usr/local/include/faudes

#######################################################

# Final build stage that contains all dependencies to build LoAT itself
FROM base as loat-base

COPY --from=z3 /usr/local/lib64/libz3.a /usr/local/lib64/
COPY --from=z3 /usr/local/include/z3*.h /usr/local/include/
COPY --from=z3 /usr/local/bin/z3 /usr/local/bin/z3

COPY --from=poly /usr/local/include/poly /usr/local/include/poly
COPY --from=poly /usr/local/lib/libpoly.a /usr/local/lib/
COPY --from=poly /usr/local/lib/libpolyxx.a /usr/local/lib/

COPY --from=cudd /usr/local/include/cudd.h /usr/local/include/cudd.h
COPY --from=cudd /usr/local/lib/libcudd.a /usr/local/lib/libcudd.a

COPY --from=ntl /usr/local/include/NTL /usr/local/include/NTL
COPY --from=ntl /usr/local/lib/libntl.a /usr/local/lib/libntl.a

COPY --from=yices /usr/local/include/yices*.h /usr/local/include/
COPY --from=yices /usr/local/lib/libyices.a /usr/local/lib/libyices.a

COPY --from=ginac /usr/local/lib64/libginac.a /usr/local/lib64/libginac.a
COPY --from=ginac /usr/local/include/ginac /usr/local/include/ginac

COPY --from=purrs /usr/local/lib/libpurrs.a /usr/local/lib/libpurrs.a
COPY --from=purrs /usr/local/include/purrs.hh /usr/local/include/purrs.hh

COPY --from=antlr4 /usr/local/lib/libantlr4-runtime.a /usr/local/lib/libantlr4-runtime.a
COPY --from=antlr4 /usr/local/include/antlr4-runtime/ /usr/local/include/

COPY --from=faudes /usr/local/lib/libfaudes.a /usr/local/lib/libfaudes.a
COPY --from=faudes /usr/local/include/faudes /usr/local/include/faudes

COPY --from=cvc5 /usr/local/lib64/libcvc5.a /usr/local/lib/libcvc5.a
COPY --from=cvc5 /usr/local/include/cvc5 /usr/local/include/cvc5
COPY --from=cvc5 /usr/local/lib64/libcadical.a /usr/local/lib/libcadical.a

COPY --from=swine /swine-z3/include/config.h /usr/local/include/swine/
COPY --from=swine /swine-z3/include/lemma_kind.h /usr/local/include/swine/
COPY --from=swine /swine-z3/include/preproc_kind.h /usr/local/include/swine/
COPY --from=swine /swine-z3/include/swine.h /usr/local/include/swine/
COPY --from=swine /swine-z3/build/libswine-z3.a /usr/local/lib/
