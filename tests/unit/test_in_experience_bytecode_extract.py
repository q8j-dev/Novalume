import pathlib
import sys
import unittest


TOOLS = pathlib.Path(__file__).parents[2] / "tools" / "fixture_inspector"
sys.path.insert(0, str(TOOLS))

from extract_in_experience_bytecode import safe_relative_path
from in_experience_contracts import FormatError


class InExperienceBytecodeExtractTests(unittest.TestCase):
    def test_prefix_is_removed_and_extension_is_normalized(self):
        self.assertEqual(
            safe_relative_path("Root/Package/Module", "Root/Package"),
            pathlib.Path("Module.lua"),
        )

    def test_parent_traversal_is_rejected(self):
        with self.assertRaises(FormatError):
            safe_relative_path("Root/Package/../Escape", "Root/Package")

    def test_dotted_module_names_do_not_collide(self):
        self.assertEqual(
            safe_relative_path("Root/Package/jest.config", "Root/Package"),
            pathlib.Path("jest.config.lua"),
        )


if __name__ == "__main__":
    unittest.main()
