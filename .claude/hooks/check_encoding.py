#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PostToolUse hook: 新建/未跟踪的 code/*.c|*.h 含中文但非 GB2312 时报警。

分工：
  - 本 hook 只管"新建文件忘了转码"（未跟踪文件）。
  - 已跟踪文件被损坏（如 sed 误改）由 encoding-auditor subagent 兜底。
  - 跳过已跟踪文件：项目所有 code/ 源文件已统一为 GB2312，
    但对已跟踪文件的 Edit 不二次猜测其编码，避免误报。

反馈机制：PostToolUse 下 exit 2 非阻塞（工具已执行无法撤销），
stderr 会回传给 Claude → Claude 看到后主动转码。
"""
import sys
import os
import json
import subprocess


def has_cjk(text):
    return any(0x4E00 <= ord(c) <= 0x9FFF for c in text)


def main():
    try:
        data = json.load(sys.stdin)
    except Exception:
        sys.exit(0)  # 输入异常不阻塞

    file_path = (data.get("tool_input") or {}).get("file_path", "")
    if not file_path:
        sys.exit(0)

    # 仅检查 code/ 下的 .c/.h（用路径段判断，兼容绝对/相对路径）
    norm = file_path.replace("\\", "/")
    if not (norm.endswith(".c") or norm.endswith(".h")):
        sys.exit(0)
    if "code" not in norm.split("/"):
        sys.exit(0)
    if not os.path.isfile(file_path):
        sys.exit(0)

    # 跳过 git 已跟踪文件（编码已固定，不二次猜测）
    tracked = subprocess.run(
        ["git", "ls-files", "--error-unmatch", file_path],
        capture_output=True,
    )
    if tracked.returncode == 0:
        sys.exit(0)  # 已跟踪，放行

    # 读取文件内容
    try:
        raw = open(file_path, "rb").read()
    except OSError:
        sys.exit(0)

    # 无 CJK（纯 ASCII/符号）→ 不关心编码
    try:
        text_utf8 = raw.decode("utf-8")
    except UnicodeDecodeError:
        text_utf8 = None
    if text_utf8 is None or not has_cjk(text_utf8):
        sys.exit(0)

    # 含 CJK：要求能按 GB2312 解码
    try:
        raw.decode("gb2312")
        sys.exit(0)  # 已合规 GB2312
    except UnicodeDecodeError:
        pass

    # 未跟踪 + 含中文 + 非 GB2312 → 强提醒
    sys.stderr.write(
        "⚠ {f}\n"
        "  含中文但为 UTF-8 编码，本项目要求源文件 GB2312（Keil 按 GBK 解析）。\n"
        "  请用 add-peripheral-module skill 的「GB2312 编码工作流」转码：\n"
        "    python3 -c \"t=open('{f}','rb').read().decode('utf-8');"
        "open('{f}','wb').write(t.encode('gbk'))\"\n"
        "  （gbk 是 gb2312 超集，兜底最稳）\n"
        .format(f=file_path)
    )
    sys.exit(2)


if __name__ == "__main__":
    main()
