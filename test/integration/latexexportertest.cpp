// 文件说明：test\integration\latexexportertest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Copyright 2026
 */
#include "latexexportertest.h"

#include <QtTest>

#include <export/latexexporter.h>

// 函数说明：实现 LaTeXExporterTest::preservesInlineAndDisplayMath 的核心逻辑，供当前模块调用。
void LaTeXExporterTest::preservesInlineAndDisplayMath()
{
    LaTeXExporter exporter;
    const QString markdown = QStringLiteral("Inline $x+1$ and display $$x^2$$.");

    const QString latex = exporter.convertToLaTeX(markdown);

    QVERIFY(latex.contains("$x+1$"));
    QVERIFY(latex.contains("$$x^2$$"));
    QVERIFY(!latex.contains("\\$x+1\\$"));
    QVERIFY(!latex.contains("\\$\\$x^2\\$\\$"));
}

// 函数说明：实现 LaTeXExporterTest::ignoresEscapedDollarAndCodeSpanForMath 的核心逻辑，供当前模块调用。
void LaTeXExporterTest::ignoresEscapedDollarAndCodeSpanForMath()
{
    LaTeXExporter exporter;
    const QString markdown = QStringLiteral("Price is \\$100, code is `$not_math$`, math is $y$.");

    const QString latex = exporter.convertToLaTeX(markdown);

    QVERIFY(latex.contains("\\$100"));
    QVERIFY(latex.contains("\\texttt{\\$not\\_math\\$}"));
    QVERIFY(latex.contains("$y$"));
}

// 函数说明：实现 LaTeXExporterTest::usesListingsInAutoModeByDefault 的核心逻辑，供当前模块调用。
void LaTeXExporterTest::usesListingsInAutoModeByDefault()
{
    qunsetenv("CUTEMARKED_LATEX_MINTED");

    LaTeXExporter exporter;
    LaTeXExporter::DocumentOptions options;
    options.codeHighlight = LaTeXExporter::CodeHighlight::Auto;
    exporter.setOptions(options);

    const QString markdown = QStringLiteral("```cpp\nint value = 1;\n```\n");
    const QString latex = exporter.convertToLaTeX(markdown);

    QVERIFY(latex.contains("\\usepackage{listings}"));
    QVERIFY(!latex.contains("\\usepackage{minted}"));
    QVERIFY(latex.contains("\\begin{lstlisting}[language=C++]"));
}

// 函数说明：实现 LaTeXExporterTest::usesMintedInAutoModeWhenEnvironmentEnabled 的核心逻辑，供当前模块调用。
void LaTeXExporterTest::usesMintedInAutoModeWhenEnvironmentEnabled()
{
    qputenv("CUTEMARKED_LATEX_MINTED", "1");

    LaTeXExporter exporter;
    LaTeXExporter::DocumentOptions options;
    options.codeHighlight = LaTeXExporter::CodeHighlight::Auto;
    exporter.setOptions(options);

    const QString markdown = QStringLiteral("```cpp\nint value = 1;\n```\n");
    const QString latex = exporter.convertToLaTeX(markdown);

    QVERIFY(latex.contains("\\usepackage{minted}"));
    QVERIFY(!latex.contains("\\usepackage{listings}"));
    QVERIFY(latex.contains("\\begin{minted}{cpp}"));

    qunsetenv("CUTEMARKED_LATEX_MINTED");
}

