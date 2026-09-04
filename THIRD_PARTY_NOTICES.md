# Third-Party Notices

The top-level MIT license applies only to code authored for this repository by
the `aix402` project. Third-party components keep their own copyright notices
and license terms.

## luatos-cli

- Location: `tools/luatos-cli`
- Project: https://github.com/wendal/luatos-cli
- License: MIT
- License text: `tools/luatos-cli/LICENSE`

`luatos-cli` is included as a Git submodule. Its repository contains additional
third-party components with their own terms, including:

- `sftool-lib`: Apache-2.0
- Lua 5.3: MIT
- littlefs: BSD-3-Clause

Refer to the files and notices inside `tools/luatos-cli` for the authoritative
copyright and license text of those components.

## Maintainer-Only Build Dependencies

The following source trees are intentionally kept outside this public
repository in the maintainer workspace. They are not covered by this
repository's top-level license:

- `LuatOS`: MIT, copyright notices retained by the upstream project
  (https://gitee.com/openLuat/LuatOS)
- `luatos-soc-2024`: MIT, copyright notices retained by the upstream project
  (the source URL is recorded in the maintainer's dependency setup)

When release packages contain binaries or files built from these dependencies,
the applicable upstream notices and license texts must accompany the release.

## Other Tools

Development tools such as Arduino CLI, xmake, and the GNU Arm toolchain are
not relicensed by this repository. Their own license terms apply to the
corresponding tool distributions.
