from __future__ import annotations

import pathlib
import sys
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools" / "fixture_inspector"
sys.path.insert(0, str(TOOLS))

from in_experience_engine_contracts import (
    FormatError,
    UI_CLASSES,
    inspect_model,
    source_property_contracts,
)


class InExperienceEngineContractsTests(unittest.TestCase):
    def test_required_native_ui_classes_are_in_scope(self) -> None:
        self.assertTrue(
            {
                "CanvasGroup",
                "UICorner",
                "UIFlexItem",
                "UIGradient",
                "UIListLayout",
                "UIPadding",
                "UIScale",
                "UISizeConstraint",
                "UIStroke",
                "UITextSizeConstraint",
                "UIDragDetector",
                "UIPageLayout",
                "VideoFrame",
                "ViewportFrame",
            }.issubset(UI_CLASSES)
        )

    def test_missing_decompiled_tree_fails_instead_of_emitting_empty_report(self) -> None:
        with self.assertRaisesRegex(FormatError, "does not exist"):
            inspect_model(ROOT / "tests" / "fixtures" / "missing.rbxm", ROOT / "missing")

    def test_source_property_contracts_follow_literal_elements_and_instances(self) -> None:
        contracts = source_property_contracts(
            """
            local page = Instance.new("UIPageLayout")
            page.Circular = true
            page.TweenTime = 0.25
            return React.createElement("Frame", {
                AnchorPoint = Vector2.new(0.5, 0.5),
                Size = UDim2.fromOffset(100, 40),
                Nested = makeValue({ Ignored = true }),
                [React.Event.Activated] = callback,
            })
            """
        )
        self.assertEqual(contracts["UIPageLayout"]["Circular"], 1)
        self.assertEqual(contracts["UIPageLayout"]["TweenTime"], 1)
        self.assertEqual(contracts["Frame"]["AnchorPoint"], 1)
        self.assertEqual(contracts["Frame"]["Size"], 1)
        self.assertNotIn("Ignored", contracts["Frame"])


if __name__ == "__main__":
    unittest.main()
