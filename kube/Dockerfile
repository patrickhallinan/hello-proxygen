FROM ubuntu:24.04

RUN apt-get update
RUN apt install -y gcc g++ gdb
RUN apt-get install -y clang libfast-float-dev
RUN apt-get install -y git tmux sudo vim
RUN apt-get install -y openssh-server

# Create the SSH directory and set permissions
# TODO Delete this and test
RUN mkdir /var/run/sshd
RUN chmod 0755 /var/run/sshd

RUN sed -i 's/#PermitRootLogin prohibit-password/PermitRootLogin no/' /etc/ssh/sshd_config
RUN sed -i 's/#PasswordAuthentication yes/PasswordAuthentication yes/' /etc/ssh/sshd_config

# not using `git clone --recurse-submodules` because no github keys

WORKDIR /
RUN git clone https://github.com/patrickhallinan/hello-proxygen

WORKDIR /hello-proxygen
RUN git clone https://github.com/facebook/proxygen.git

# Add user and set password (for simplicity, don't do this in production; use keys)
#RUN useradd -m -s /bin/bash ubuntu
RUN echo 'ubuntu:pass' | chpasswd
RUN echo 'root:pass' | chpasswd

RUN chown -R ubuntu:ubuntu /hello-proxygen/

# passwordless sudo
RUN echo "ubuntu ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers.d/passwordless_sudo
RUN chmod 0440 /etc/sudoers.d/passwordless_sudo

USER ubuntu
WORKDIR /hello-proxygen/proxygen/proxygen/
RUN git checkout fd53add59

RUN ./build.sh

# build hello
WORKDIR /hello-proxygen/
RUN mkdir build
WORKDIR /hello-proxygen/build
RUN cmake ..
RUN make


# Expose SSH port
EXPOSE 22

# Start SSH service
CMD ["/usr/sbin/sshd", "-D"]
