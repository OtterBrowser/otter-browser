(function (window)
{
	var image = document.querySelector('img');
	var ignore = false;

	if (image && !image.classList.contains('imageViewer'))
	{
		image.classList.add('imageViewer');
		image.addEventListener('click', function(event)
		{
			image.removeAttribute('width');
			image.removeAttribute('height');
			image.removeAttribute('style');

			var documentSize = [document.documentElement.clientWidth, document.documentElement.clientHeight];
			var imageSize = [image.naturalWidth, image.naturalHeight];

			if (imageSize[0] <= documentSize[0] && imageSize[1] <= documentSize[1])
			{
				image.classList.remove('zoomedIn');
				image.classList.remove('zoomedOut');
			}
			else if (!ignore)
			{
				if (image.classList.contains('zoomedOut'))
				{
					image.classList.add('zoomedIn');
					image.classList.remove('zoomedOut');
				}
				else
				{
					image.classList.add('zoomedOut');
					image.classList.remove('zoomedIn');
				}
			}

			ignore = false;
		});
		image.click();

		this.addEventListener('resize', function()
		{
			ignore = true;
			image.click();
		});
	}
})();
