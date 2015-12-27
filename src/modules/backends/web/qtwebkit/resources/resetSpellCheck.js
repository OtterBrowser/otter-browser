var selection = document.getSelection();
var selectionStart = this.selectionStart;
var selectionEnd = this.selectionEnd;
var selectionDirection = this.selectionDirection;

selection.modify('move', 'backward', 'documentboundary');

for (var i = 0; i < this.value.length; ++i)
{
	selection.modify('move', 'forward', 'character');
}

selection.removeAllRanges();

this.setSelectionRange(selectionStart, selectionEnd, selectionDirection);
