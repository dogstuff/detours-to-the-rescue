# syntax=docker/dockerfile:1.7
FROM docker.io/nixos/nix:2.24.11

RUN printf '%s\n' \
  'experimental-features = nix-command flakes' \
  'filter-syscalls = false' \
  'sandbox = false' \
  'auto-optimise-store = false' \
  >> /etc/nix/nix.conf
WORKDIR /workspace

COPY flake.nix flake.lock /workspace/

RUN nix develop --profile /nix/var/nix/profiles/dttr-default \
      ".#devShells.$(nix eval --impure --raw --expr builtins.currentSystem).default" --command true \
 && nix-collect-garbage -d \
 && rm -rf /root/.cache/nix

ENTRYPOINT ["/bin/sh", "-c", "exec nix develop -c \"$@\"", "--"]
