"""MinerU CLI adapter with stable, safe, page-oriented results."""

from __future__ import annotations

from dataclasses import dataclass
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from threading import Event
import time

from document_extraction.models import ExtractedDocument, ExtractedPage, ExtractionError


MINERU_EXTENSIONS = {".pdf", ".jpg", ".jpeg", ".png", ".gif", ".webp"}
_TEXT_BLOCK_TYPES = {"text", "list", "code", "equation"}


class MineruError(ExtractionError):
    """Safe error raised by the local MinerU adapter."""


@dataclass(frozen=True)
class MineruOptions:
    command: str = "mineru"
    api_url: str | None = None
    backend: str = "hybrid-engine"
    effort: str = "high"
    method: str = "ocr"
    formula_enabled: bool = False
    table_enabled: bool = False
    image_analysis: bool = False
    language: str | None = None
    timeout_seconds: int = 1800


class MineruExtractor:
    """Run a locally installed MinerU command; no document leaves the host."""

    def __init__(self, options: MineruOptions | None = None) -> None:
        self.options = options or MineruOptions()

    def extract(self, path: str | Path, cancellation: Event | None = None) -> ExtractedDocument:
        # MinerU is deliberately run with its working directory set to an
        # ephemeral output directory.  The upload path must therefore be
        # absolute; otherwise a relative `data/temp/...` path stops resolving.
        source = Path(path).resolve()
        if source.suffix.lower() not in MINERU_EXTENSIONS:
            raise MineruError("unsupported_document", "The document type is not supported by MinerU.")
        with tempfile.TemporaryDirectory(prefix="document-extraction-") as temp_dir:
            output_dir = Path(temp_dir)
            self._run(source, output_dir, cancellation)
            blocks = self._read_content_list(output_dir)
        return _document_from_blocks(blocks)

    def _run(self, path: Path, output_dir: Path, cancellation: Event | None = None) -> None:
        command_path = self.options.command if Path(self.options.command).is_file() else shutil.which(self.options.command)
        if not command_path:
            raise MineruError("extractor_unavailable", "The configured MinerU command is not available.")
        command = [
            str(command_path), "--path", str(path), "--output", str(output_dir),
            "--backend", self.options.backend, "--method", self.options.method,
            "--effort", self.options.effort, "--formula", str(self.options.formula_enabled).lower(),
            "--table", str(self.options.table_enabled).lower(),
            "--image-analysis", str(self.options.image_analysis).lower(),
        ]
        if self.options.language:
            command.extend(["--lang", self.options.language])
        if self.options.api_url:
            command.extend(["--api-url", self.options.api_url])
        # Some MinerU versions create an `output/` directory relative to their
        # current working directory despite receiving `--output`.  Run it in
        # our TemporaryDirectory so uploads, OCR images and PDF copies vanish
        # immediately after the structured text has been read.
        process = subprocess.Popen(command, cwd=output_dir, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        deadline = time.monotonic() + self.options.timeout_seconds
        while process.poll() is None:
            if cancellation and cancellation.is_set():
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                raise MineruError("extractor_cancelled", "The local extraction was cancelled.")
            if time.monotonic() >= deadline:
                process.kill()
                process.wait()
                raise MineruError("extractor_timeout", "The local extractor exceeded its configured timeout.")
            time.sleep(0.05)
        stdout, stderr = process.communicate()
        result = type("Result", (), {"returncode": process.returncode, "stdout": stdout, "stderr": stderr})()
        if result.returncode:
            raise MineruError("extractor_failed", "The local extractor failed.")

    def _read_content_list(self, output_dir: Path) -> list[object]:
        candidates = sorted(output_dir.rglob("*_content_list.json"))
        if not candidates:
            raise MineruError("invalid_extractor_output", "The local extractor returned no structured output.")
        try:
            blocks = json.loads(candidates[0].read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            raise MineruError("invalid_extractor_output", "The local extractor returned invalid structured output.") from exc
        if not isinstance(blocks, list):
            raise MineruError("invalid_extractor_output", "The local extractor returned invalid structured output.")
        return blocks


def _document_from_blocks(blocks: list[object]) -> ExtractedDocument:
    page_parts: dict[int, list[str]] = {}
    for block in blocks:
        if not isinstance(block, dict) or str(block.get("type", "")).lower() not in _TEXT_BLOCK_TYPES:
            continue
        text = str(block.get("text", "")).strip()
        if text:
            page_parts.setdefault(int(block.get("page_idx", 0)), []).append(text)
    pages = [
        ExtractedPage(page_number=index + 1, text="\n\n".join(parts), metadata={"mineru_page_index": index})
        for index, parts in sorted(page_parts.items())
    ]
    return ExtractedDocument(pages=pages, extractor="mineru")
