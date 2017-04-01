var requests = [%1];
var elements = Array.from(document.querySelectorAll('[src]'));

for (var i = 0; i < elements.length; ++i)
{
	for (var j = 0; j < requests.length; ++j)
	{
		if (elements[i].getAttribute('src').indexOf(requests[j]) > -1)
		{
			elements[i].style.cssText = 'display:none !important';
		}
	}
}
