var value = this.value;
var selectionStart = this.selectionStart;
var selectionEnd = this.selectionEnd;
var selectionDirection = this.selectionDirection;

this.value = '';
this.value = value;
this.setSelectionRange(selectionStart, selectionEnd, selectionDirection);
