file(REMOVE_RECURSE
  "doxygen"
  "html"
  "pdf"
  "sphinx/_build"
  "sphinx/_doctrees"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/doc.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
