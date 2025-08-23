# Use the same Ubuntu version as the GitHub Actions runner
FROM ubuntu:24.04 as base

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install all dependencies from all jobs (test, coverage, sanitizers)
RUN apt-get update && apt-get install -y \
    build-essential \
    ccache \
    cmake \
    curl \
    pkg-config \

    git \
    # For OpenGL headless rendering
    libgl1-mesa-dev \
    libgles2-mesa-dev \
    libegl1-mesa-dev \
    # X11 libraries for xvfb
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libxext-dev \
    xvfb \
    # For networking and images
    libbrotli-dev \
    libcurl4-openssl-dev \
    libglfw3-dev \
    libidn2-dev \
    libjpeg-dev \
    libnghttp2-dev \
    libpng-dev \
    libpsl-dev \
    libssl-dev \
    libuv1-dev \
    libwebp-dev \
    libzstd-dev \
    libunistring-dev \
    meson \
    zlib1g-dev \
    # For coverage and sanitizers jobs
    gcc \
    lcov \
    clang \
    && rm -rf /var/lib/apt/lists/*

# Install Rust
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV CARGO_HOME=/root/.cargo
ENV RUSTUP_HOME=/root/.rustup
ENV PATH="${CARGO_HOME}/bin:${PATH}"

# --- End of base stage for CI ---

# Set the working directory for local builds
WORKDIR /work

# Copy the source code
FROM base as final
COPY . .

# Initialize and update submodules
# This requires the .git directory to be copied into the container.
# Ensure your .dockerignore file does not exclude it.
RUN git submodule update --init --recursive

# Build the project
RUN cmake -B build -S .
RUN cmake --build build -j$(nproc) --verbose

# Run the tests
RUN cd build && xvfb-run -a ctest --output-on-failure

# Set the default command to run the example
CMD ["./build/examples/slint-maplibre-example"]
