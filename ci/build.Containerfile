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

RUN for shell in ci-shaders ci-build ci-test ci-docs ci-package ci-upload; do \
      nix develop --profile "/nix/var/nix/profiles/dttr-${shell}" \
        ".#devShells.x86_64-linux.${shell}" --command true; \
    done \
 && nix-collect-garbage -d \
 && rm -rf /root/.cache/nix
