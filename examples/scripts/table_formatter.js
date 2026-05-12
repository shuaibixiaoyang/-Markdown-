// @name Markdown 表格格式化
// @description 选中 Markdown 表格文本，自动对齐列宽
// @author CuteMarkEd
// @version 1.0
// @shortcut Ctrl+Shift+T

var text = editor.selectedText;
if (!text) {
    app.showWarning('表格格式化', '请先选中要格式化的 Markdown 表格');
} else {
    var lines = text.split('\n').filter(function(l) { return l.trim().length > 0; });

    if (lines.length < 2) {
        app.showWarning('表格格式化', '需要至少 2 行（表头 + 分隔行）');
    } else {
        // 解析每行的单元格
        var rows = [];
        for (var i = 0; i < lines.length; i++) {
            var cells = lines[i].split('|')
                .map(function(c) { return c.trim(); })
                .filter(function(c, idx, arr) {
                    // 去掉首尾空单元格（因为 |col1|col2| 拆分后首尾是空的）
                    return !(idx === 0 && c === '') && !(idx === arr.length - 1 && c === '');
                });
            rows.push(cells);
        }

        // 计算每列最大宽度（考虑中文字符占2个宽度）
        var colCount = 0;
        for (var i = 0; i < rows.length; i++) {
            if (rows[i].length > colCount) colCount = rows[i].length;
        }

        function displayWidth(str) {
            var w = 0;
            for (var i = 0; i < str.length; i++) {
                w += str.charCodeAt(i) > 127 ? 2 : 1;
            }
            return w;
        }

        function padRight(str, width) {
            var diff = width - displayWidth(str);
            if (diff <= 0) return str;
            return str + util.repeat(' ', diff);
        }

        var colWidths = [];
        for (var c = 0; c < colCount; c++) {
            var maxW = 3; // 最小宽度 ---
            for (var r = 0; r < rows.length; r++) {
                if (r === 1) continue; // 跳过分隔行
                var cell = rows[r][c] || '';
                var w = displayWidth(cell);
                if (w > maxW) maxW = w;
            }
            colWidths.push(maxW);
        }

        // 重建表格
        var result = [];
        for (var r = 0; r < rows.length; r++) {
            var line = '|';
            for (var c = 0; c < colCount; c++) {
                if (r === 1) {
                    // 分隔行
                    line += ' ' + util.repeat('-', colWidths[c]) + ' |';
                } else {
                    var cell = rows[r][c] || '';
                    line += ' ' + padRight(cell, colWidths[c]) + ' |';
                }
            }
            result.push(line);
        }

        editor.replaceSelection(result.join('\n'));
        app.showMessage('表格已格式化（' + colCount + ' 列，' + rows.length + ' 行）', 3000);
    }
}
