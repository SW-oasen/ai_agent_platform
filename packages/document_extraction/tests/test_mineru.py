from pathlib import Path

import pytest
import document_extraction.mineru as mineru_module

from document_extraction import MineruError, MineruExtractor, MineruOptions


def test_mineru_adapter_returns_ordered_page_results(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    source = tmp_path / "scan.pdf"
    source.write_bytes(b"%PDF")
    extractor = MineruExtractor()

    def fake_run(path: Path, output_dir: Path, cancellation=None) -> None:
        output = output_dir / "result_content_list.json"
        output.write_text(
            '[{"type":"text","text":"Second","page_idx":1},'
            '{"type":"image","text":"ignored","page_idx":0},'
            '{"type":"text","text":"First","page_idx":0}]',
            encoding="utf-8",
        )

    monkeypatch.setattr(extractor, "_run", fake_run)
    result = extractor.extract(source)

    assert [(page.page_number, page.text) for page in result.pages] == [(1, "First"), (2, "Second")]
    assert result.text == "First\n\nSecond"


def test_mineru_adapter_reports_missing_command_without_document_name(tmp_path: Path) -> None:
    source = tmp_path / "private-client-file.pdf"
    source.write_bytes(b"%PDF")

    with pytest.raises(MineruError) as error:
        MineruExtractor(MineruOptions(command="definitely-not-installed-mineru")).extract(source)

    assert error.value.code == "extractor_unavailable"
    assert "private-client-file" not in str(error.value)


def test_mineru_adapter_uses_configured_local_api(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    source = tmp_path / "scan.pdf"
    source.write_bytes(b"%PDF")
    seen: list[str] = []

    class Process:
        returncode = 0
        def poll(self): return 0
        def communicate(self): return "", ""

    monkeypatch.setattr(mineru_module.shutil, "which", lambda _: "mineru")
    captured = {}
    def fake_popen(command, **kwargs):
        captured.update(kwargs)
        seen.extend(command)
        return Process()
    monkeypatch.setattr(mineru_module.subprocess, "Popen", fake_popen)
    MineruExtractor(MineruOptions(api_url="http://127.0.0.1:8000"))._run(source, tmp_path)

    assert seen[-2:] == ["--api-url", "http://127.0.0.1:8000"]
    assert captured["cwd"] == tmp_path


def test_mineru_resolves_relative_upload_path_before_changing_working_directory(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    source = tmp_path / "scan.jpg"
    source.write_bytes(b"image")
    extractor = MineruExtractor(MineruOptions(command="mineru"))
    seen = []

    def fake_run(path: Path, output_dir: Path, cancellation=None) -> None:
        seen.append(path)
        (output_dir / "result_content_list.json").write_text("[]", encoding="utf-8")

    monkeypatch.setattr(extractor, "_run", fake_run)
    monkeypatch.chdir(tmp_path)
    extractor.extract(Path("scan.jpg"))

    assert seen[0].is_absolute()
