file(REMOVE_RECURSE
  "doxygen"
  "html"
  "pdf"
  "sphinx/_build"
  "sphinx/_doctrees"
  "CMakeFiles/sphinx-LATEX"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/sphinx-LATEX.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
