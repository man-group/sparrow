# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

FROM apache/arrow-dev:amd64-conda-integration

ENV ARROW_USE_CCACHE=OFF \
    ARROW_CPP_EXE_PATH=/build/cpp/debug \
    ARROW_NANOARROW_PATH=/build/nanoarrow \
    BUILD_DOCS_CPP=OFF \
    ARROW_INTEGRATION_CPP=ON \
    ARROW_INTEGRATION_CSHARP=OFF \
    ARROW_INTEGRATION_GO=OFF \
    ARROW_INTEGRATION_JAVA=OFF \
    ARROW_INTEGRATION_JS=OFF \
    ARCHERY_INTEGRATION_WITH_NANOARROW="1" \
    ARCHERY_INTEGRATION_WITH_RUST="0"

ENV ARROW_USE_CCACHE=OFF
ENV ARROW_CPP_EXE_PATH=/build/cpp/debug
ENV ARROW_NANOARROW_PATH=/build/nanoarrow
ENV BUILD_DOCS_CPP=OFF

RUN apt update

RUN apt install build-essential git -y

# Clone the arrow monorepo // TODO: change to the official repo
RUN git clone --depth 1 --branch try_both_fix https://github.com/Alex-PLACET/arrow.git /arrow-integration --recurse-submodules

# Clone the nanoarrow repo
RUN git clone --depth 1 --branch apache-arrow-nanoarrow-0.7.0 https://github.com/apache/arrow-nanoarrow /arrow-integration/nanoarrow

# Build all the integrations
RUN conda run --no-capture-output \
    /arrow-integration/ci/scripts/integration_arrow_build.sh \
    /arrow-integration \
    /build
