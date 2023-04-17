file(REMOVE_RECURSE
  "doxygen"
  "html"
  "pdf"
  "sphinx/_build"
  "sphinx/_doctrees"
  "CMakeFiles/doxygen"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/doxygen.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
