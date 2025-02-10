FROM ubuntu:latest

RUN apt update 
#RUN apt install -y gcc g++ gdb
#RUN apt install -y clang libfast-float-dev
#RUN apt install -y openssh-server 

#RUN useradd -m -s ubuntu
#RUN echo 'ubuntu:password' | chpasswd

COPY ./hello/hello /app/
WORKDIR /app

# Install necessary packages, including ssh
RUN apt-get update && \
    apt-get install -y openssh-server && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Create the SSH directory and set permissions
RUN mkdir /var/run/sshd
RUN chmod 0755 /var/run/sshd

# Add user and set password (for simplicity, don't do this in production; use keys)
#RUN useradd -m -s /bin/bash ubuntu
RUN echo 'ubuntu:pass' | chpasswd

# Enable password authentication for SSH (not recommended for production)
RUN sed -i 's/#PermitRootLogin prohibit-password/PermitRootLogin no/' /etc/ssh/sshd_config && \
    sed -i 's/#PasswordAuthentication yes/PasswordAuthentication yes/' /etc/ssh/sshd_config

# Expose SSH port
EXPOSE 22

# Start SSH service
CMD ["/usr/sbin/sshd", "-D"]
