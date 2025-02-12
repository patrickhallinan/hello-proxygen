FROM ubuntu:24.04

ARG PUBLICKEY
ARG PRIVATEKEY

RUN echo "PUB: $PUBLICKEY, PRIVATE: $PRIVATEKEY"

COPY ./hello/hello /app/
WORKDIR /app

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

# change user to ubuntu

checkout correct version of proxygen

# build proxygen


# build hello


# Expose SSH port
EXPOSE 22

# Start SSH service
CMD ["/usr/sbin/sshd", "-D"]
