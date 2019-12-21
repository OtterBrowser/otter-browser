var importBookmarksObj;

function processBookmarkItems(items) {
    var currentIndex = 0;

    while (currentIndex < items.length) {
        var node = items[currentIndex];
        currentIndex += 1;

        if (node.tagName.toUpperCase() === 'A')
        {
            if (node.innerHTML === '---' && node.getAttribute('href') === 'http://bookmark.placeholder.url/')
            {
                // vivaldi markup for separators
                importBookmarksObj.addBookmark({'type':'separator'});
            }
            else
            {
                importBookmarksObj.addBookmark({
                                                   'type': (node.hasAttribute('FEEDURL') ? 'feed' : 'anchor'),
                                                   'title': node.innerHTML,
                                                   'url': node.getAttribute('href'),
                                                   'dateAdded': node.getAttribute('add_date'),
                                                   'dateModified': node.getAttribute('last_modified'),
                                                   'keyword': node.getAttribute('shortcuturl')
                                               });
            }
        }
        else if (node.tagName.toUpperCase() === 'HR')
        {
            importBookmarksObj.addBookmark({'type':'separator'});
        }
        else if (node.tagName.toUpperCase() === 'H3')
        {
            directChildren = node.parentNode.querySelectorAll(':scope > DL > DT > *, :scope > DL > HR');

            importBookmarksObj.beginFolder({
                                               type: 'folder',
                                               title: node.innerHTML,
                                               dateAdded: node.getAttribute('add_date'),
                                               dateModified: node.getAttribute('last_modified')
                                           });
            processBookmarkItems(directChildren);
            importBookmarksObj.endFolder();
        }
    }
}

new QWebChannel(qt.webChannelTransport, function(webChannel) {
    importBookmarksObj = webChannel.objects.bookmarkImporter;

    var totalAmount = document.querySelectorAll('DT, HR').length;

    var estimatedUrls = document.querySelectorAll('A[href]').length;
    var estimatedKeywords = document.querySelectorAll('A[shortcuturl]').length;

    importBookmarksObj.beginImport(totalAmount, estimatedUrls, estimatedKeywords);

    var rootNode = document.querySelector('dl');
    var directChildren = rootNode.querySelectorAll(':scope > DT > *, :scope > HR');

    processBookmarkItems(directChildren);
    importBookmarksObj.endImport();
});
