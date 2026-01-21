# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import os
import sys
sys.path.insert(0, os.path.abspath('../../main-app'))

project = 'Platforma_PBL_App'
copyright = '2026, Miłosz Liniewiecki, Magdalena Rąpała, Jakub Gomola, Michał Świątczak'
author = 'Miłosz Liniewiecki, Magdalena Rąpała, Jakub Gomola, Michał Świątczak'
release = '2026'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',   # pulls docstrings from your Python code
    'sphinx.ext.viewcode',  # adds clickable links to source
    'sphinx.ext.napoleon',  # supports Google/NumPy style docstrings
]

templates_path = ['_templates']
exclude_patterns = []



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
#html_theme = 'alabaster'
html_static_path = ['_static']

