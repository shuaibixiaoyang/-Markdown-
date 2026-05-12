// @name 批量图片转换
// @description 将选中文本中的图片 URL 批量转换为 Markdown 图片语法
// @author CuteMarkEd
// @version 1.0

var text = editor.selectedText;
if (!text) {
    app.showWarning('图片转换', '请先选中包含图片 URL 的文本（每行一个 URL）');
} else {
    var lines = text.split('\n');
    var result = [];
    var converted = 0;

    for (var i = 0; i < lines.length; i++) {
        var line = lines[i].trim();
        if (line.length === 0) {
            result.push('');
            continue;
        }

        // 如果已经是 Markdown 图片语法，保持不变
        if (line.match(/^!\[.*\]\(.*\)$/)) {
            result.push(line);
            continue;
        }

        // 匹配图片 URL
        if (line.match(/\.(png|jpg|jpeg|gif|bmp|svg|webp)(\?.*)?$/i) ||
            line.match(/^https?:\/\/.*\.(png|jpg|jpeg|gif|bmp|svg|webp)/i)) {
            // 从 URL 中提取文件名作为 alt text
            var fileName = line.split('/').pop().split('?')[0];
            var altText = fileName.replace(/\.[^.]+$/, '').replace(/[-_]/g, ' ');
            result.push('![' + altText + '](' + line + ')');
            converted++;
        } else if (line.match(/^https?:\/\//)) {
            // 普通 URL 转为链接
            result.push('[链接](' + line + ')');
            converted++;
        } else {
            result.push(line);
        }
    }

    editor.replaceSelection(result.join('\n'));
    app.showMessage('已转换 ' + converted + ' 个链接', 3000);
}
