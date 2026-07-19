#!/usr/bin/env python3

import unittest
from pathlib import Path


ROOT = Path(__file__).parents[2]


class BuilderIconContractTests(unittest.TestCase):
    def test_adapter_uses_exact_ligature_and_face_contract(self) -> None:
        source = (ROOT / "content/scripts/Modules/InExperience/BuilderIcon.lua").read_text()
        self.assertIn('icon.Text = name', source)
        self.assertIn('Enum.Font.BuilderIconsRegular', source)
        self.assertIn('Enum.Font.BuilderIconsFilled', source)
        self.assertNotIn('Image =', source)
        self.assertNotIn('rbxassetid://', source)

    def test_chrome_tokens_match_recovered_desktop_contract(self) -> None:
        source = (ROOT / "content/scripts/Modules/InExperience/Tokens.lua").read_text()
        for contract in (
            "Size_1100 = 44",
            "Size_900 = 36",
            "Size_700 = 28",
            "Size_600 = 24",
            "Size_1400 = 56",
            "Transparency = 0.3",
            "Color3.fromRGB(18, 18, 21)",
        ):
            self.assertIn(contract, source)


if __name__ == "__main__":
    unittest.main()
