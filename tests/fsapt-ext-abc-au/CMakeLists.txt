include(TestingMacros)

add_regression_test(fsapt-ext-abc-au "psi;quicktests;sapt;cart")
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../../psi4/share/psi4/fsapt/fsapt.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
