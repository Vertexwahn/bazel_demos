"""Fetch native Dawn and install the native Bazel overlay."""

def _dawn_repository_impl(ctx):
    ctx.download_and_extract(
        url = "https://github.com/google/dawn/archive/{}.tar.gz".format(ctx.attr.commit),
        sha256 = ctx.attr.sha256,
        stripPrefix = "dawn-{}".format(ctx.attr.commit),
    )
    python = ctx.which("python3") or ctx.which("python")
    if not python:
        fail("Fetching Dawn requires Python 3 on PATH.")
    result = ctx.execute(
        [python, ctx.path(ctx.attr._prepare)],
        timeout = 120,
    )
    if result.return_code:
        fail("Preparing Dawn failed:\n{}\n{}".format(result.stdout, result.stderr))
    ctx.file("BUILD.bazel", ctx.read(ctx.attr._build_file))

dawn_repository = repository_rule(
    implementation = _dawn_repository_impl,
    attrs = {
        "commit": attr.string(mandatory = True),
        "sha256": attr.string(mandatory = True),
        "_prepare": attr.label(default = "//bazel:prepare_dawn.py"),
        "_build_file": attr.label(default = "//bazel:dawn.BUILD.bazel"),
    },
)
