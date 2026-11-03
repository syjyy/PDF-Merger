"""Optional end-to-end checks. Python is used for tests, not the application."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from docx import Document
from openpyxl import Workbook
from PIL import Image
from pypdf import PdfReader, PdfWriter


def main():
    executable = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="ordered-office-test-") as temporary:
        root = Path(temporary)
        a, b = root / "A.pdf", root / "B.pdf"
        for path, widths in [(a, [301, 302]), (b, [401, 402, 403])]:
            writer = PdfWriter()
            for width in widths:
                writer.add_blank_page(width=width, height=600)
            writer.write(path)
        png = root / "transparent.png"
        Image.new("RGBA", (200, 100), (0, 100, 255, 128)).save(png)
        tiff = root / "two-pages.tiff"
        Image.new("RGB", (80, 120), "red").save(
            tiff, save_all=True, append_images=[Image.new("RGB", (120, 80), "blue")]
        )
        output = root / "result.pdf"
        subprocess.run([executable, "--merge", str(output), str(b), str(png), str(a), str(tiff)], check=True)
        pages = PdfReader(output).pages
        assert len(pages) == 8
        assert [int(pages[i].mediabox.width) for i in [0, 1, 2, 4, 5]] == [401, 402, 403, 301, 302]
        for index in [3, 6, 7]:
            assert abs(float(pages[index].mediabox.width) - 595.28) < 0.01
            assert abs(float(pages[index].mediabox.height) - 841.89) < 0.01
            # The image must fit inside the sheet with 18 pt margins, without stretching.
            operations = pages[index].get_contents().operations
            matrix = next(operands for operands, operator in operations if operator == b'cm')
            width, _, _, height, left, bottom = map(float, matrix)
            expected_ratio = {3: 2.0, 6: 80 / 120, 7: 120 / 80}[index]
            assert abs(width / height - expected_ratio) < 0.001
            assert left >= 17.99 and bottom >= 17.99
            assert abs(2 * left + width - 595.28) < 0.02
            assert abs(2 * bottom + height - 841.89) < 0.02
        print("PASS: PDF/image order, uniform portrait A4 images, centered uncropped aspect ratio, multi-page TIFF")
        office = os.environ.get("LIBREOFFICE_PATH", r"C:\Program Files\LibreOffice\program\soffice.exe")
        if not Path(office).is_file():
            if "--require-office" in sys.argv:
                raise RuntimeError("LibreOffice required for this test")
            print("SKIP: Office tests (LibreOffice not installed)")
            return
        os.environ["LIBREOFFICE_PATH"] = office
        docx = root / "two-pages.docx"
        document = Document()
        document.add_paragraph("WORD_PAGE_ONE")
        document.add_page_break()
        document.add_paragraph("WORD_PAGE_TWO")
        document.save(docx)
        xlsx = root / "two-sheets.xlsx"
        workbook = Workbook()
        workbook.active["A1"] = "EXCEL_SHEET_ONE"
        workbook.create_sheet("Second")["A1"] = "EXCEL_SHEET_TWO"
        workbook.save(xlsx)
        subprocess.run([executable, "--merge", str(output), str(b), str(docx), str(xlsx), str(a)], check=True)
        pages = PdfReader(output).pages
        assert len(pages) == 9, len(pages)
        assert "WORD_PAGE_ONE" in pages[3].extract_text()
        assert "WORD_PAGE_TWO" in pages[4].extract_text()
        assert "EXCEL_SHEET_ONE" in pages[5].extract_text()
        assert "EXCEL_SHEET_TWO" in pages[6].extract_text()
        assert [int(pages[i].mediabox.width) for i in [0, 1, 2, 7, 8]] == [401, 402, 403, 301, 302]
        print("PASS: Word multi-page and Excel multi-sheet conversion in whole-file order")


if __name__ == "__main__":
    main()
