FROM archlinux:base-devel

RUN echo "DisableSandbox" >> /etc/pacman.conf && \
  pacman-key --init && \
  pacman-key --populate archlinux && \
  pacman -Syu --noconfirm git sudo

RUN useradd -m -G wheel -s /bin/bash testuser && \
  echo "testuser ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

USER testuser
WORKDIR /home/testuser
RUN git clone https://aur.archlinux.org/yay-bin.git && \
  cd yay-bin && \
  makepkg -si --noconfirm

WORKDIR /home/testuser/damgr
RUN git clone https://github.com/goajos/declarative-arch-manager.git . && \
  make
