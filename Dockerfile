ARG CI_DOCKER_REGISTRY=ghcr.io/drdelabill/stm_builder:latest

FROM $CI_DOCKER_REGISTRY AS builder

ENV APP_ROOT=/app
ENV SRC_ROOT=$APP_ROOT/src
ENV DEBUG_ROOT=$APP_ROOT/Debug
ENV RELEASE_ROOT=$APP_ROOT/Release

ADD . $SRC_ROOT
RUN mkdir -p $DEBUG_ROOT \
 && mkdir -p $RELEASE_ROOT \
 && cp -f $APP_ROOT/search.cmake $SRC_ROOT/search.cmake

RUN cd $DEBUG_ROOT \
 && cmake $SRC_ROOT -DCMAKE_BUILD_TYPE=DEBUG -DDEBUG=1 \
 && cmake --build $DEBUG_ROOT

 RUN $DEBUG_ROOT/test/storageattest

RUN cd $RELEASE_ROOT \
 && cmake $SRC_ROOT \
 && cmake --build $RELEASE_ROOT

RUN $RELEASE_ROOT/test/storageattest