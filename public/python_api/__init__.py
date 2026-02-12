# Re-export everything from the native module so users can `import pyssp4sim`
from . import py_ssp4sim as _native

# Export all public attrs from the extension at the package top-level
__all__ = [n for n in dir(_native) if not n.startswith("_")]
globals().update({n: getattr(_native, n) for n in __all__})
