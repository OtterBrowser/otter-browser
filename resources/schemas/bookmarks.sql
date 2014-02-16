CREATE TABLE "bookmarks" ("id" INTEGER PRIMARY KEY, "parent" INTEGER NOT NULL, "type" INTEGER NOT NULL, "address" TEXT NOT NULL, "title" TEXT, "description" TEXT, "order" INTEGER NOT NULL, "created" INTEGER NOT NULL, "visited" INTEGER NOT NULL DEFAULT -1, "visits" INTEGER NOT NULL DEFAULT 0);
CREATE TABLE "tags" ("id" INTEGER PRIMARY KEY, "tag" TEXT UNIQUE);
CREATE TABLE "bookmarks_tags" ("bookmark" INTEGER, "tag" INTEGER, PRIMARY KEY("bookmark", "tag"));
