var element = ((%1 >= 0) ? document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)) : document.activeElement);
var result = {
	alternateText: '',
	flags: 0,
	formUrl: '',
	frameUrl: '',
	geometry: { x: -1, y: -1, w: 0, h: 0 },
	imageUrl: '',
	linkUrl: '',
	longDescription: '',
	mediaUrl: '',
	position: [%1, %2],
	tagName: '',
	title: ''
};

if (element)
{
	function createUrl(url)
	{
		var element = document.createElement('a');
		element.href = url;

		return element.href;
	}

	var geometry = element.getBoundingClientRect();
	var anchorElement = element.closest('a[href]');
	var titledElement = element.closest('[title]');

	if (anchorElement)
	{
		result.linkUrl = anchorElement.href;
	}

	if (titledElement)
	{
		result.title = titledElement.title;
	}

	result.geometry = { x: geometry.top, y: geometry.left, w: geometry.width, h: geometry.height };

	if (window.getSelection().containsNode(element, true))
	{
		result.flags |= 8;
	}

	result.tagName = element.tagName.toLowerCase();

	if (result.tagName == 'frame' || result.tagName == 'iframe')
	{
		result.frameUrl = createUrl(element.src);
		result.title = element.contentDocument.title;
	}
	else if (result.tagName == 'img')
	{
		result.alternateText = element.alt;
		result.imageUrl = createUrl(element.src);
		result.longDescription = element.longDesc;
	}
	else if (result.tagName == 'audio' || result.tagName == 'video')
	{
		if (element.controls)
		{
			result.flags |= 16;
		}

		if (element.loop)
		{
			result.flags |= 32;
		}

		if (element.muted)
		{
			result.flags |= 64;
		}

		if (element.paused)
		{
			result.flags |= 128;
		}

		result.mediaUrl = createUrl(element.src);
		result.playbackRate = element.playbackRate;
	}

	if (result.tagName == 'input' || result.tagName == 'textarea')
	{
		var type = (element.type ? element.type.toLowerCase() : '');

		if (result.tagName == 'textarea' || type == 'email' || type == 'password' || type == 'search' || type == 'tel' || type == 'text' || type == 'url')
		{
			if (!element.hasAttribute('disabled') && !element.hasAttribute('readonly'))
			{
				result.flags |= 1;
			}

			if (!element.value || element.value == '')
			{
				result.flags |= 2;
			}
		}
	}
	else if (element.isContentEditable)
	{
		result.flags |= 1;
	}

	if (result.tagName == 'button' || result.tagName == 'input' || result.tagName == 'select'|| result.tagName == 'textarea')
	{
		var formElement = element.closest('form');

		if (formElement)
		{
			var type = (element.type ? element.type.toLowerCase() : '');

			if ((result.tagName == 'button'|| result.tagName == 'input') && (type == 'image' || type == 'submit'))
			{
				result.formUrl = createUrl(element.hasAttribute('formaction') ? element.getAttribute('formaction') : formElement.action);
			}

			if (formElement.querySelectorAll('input:not([disabled])[name], select:not([disabled])[name], textarea:not([disabled])[name]').length > 0)
			{
				result.flags |= 4;
			}
		}
	}
}

result;
