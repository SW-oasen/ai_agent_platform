"""Framework-free models for local document extraction."""

from __future__ import annotations

from dataclasses import dataclass, field


class ExtractionError(RuntimeError):
    """Safe extraction failure; callers must not expose process output to clients."""

    def __init__(self, code: str, message: str) -> None:
        super().__init__(message)
        self.code = code


@dataclass(frozen=True)
class ExtractedPage:
    page_number: int
    text: str
    metadata: dict[str, object] = field(default_factory=dict)


@dataclass(frozen=True)
class ExtractedDocument:
    pages: list[ExtractedPage]
    extractor: str
    metadata: dict[str, object] = field(default_factory=dict)

    @property
    def text(self) -> str:
        return "\n\n".join(page.text for page in self.pages if page.text).strip()

