CREATE TABLE "bookmarks" ("id" INTEGER PRIMARY KEY, "parent" INTEGER NOT NULL, "type" INTEGER NOT NULL, "address" TEXT NOT NULL, "keyword" TEXT UNIQUE, "title" TEXT, "description" TEXT, "order" INTEGER NOT NULL, "created" INTEGER NOT NULL, "visited" INTEGER NOT NULL DEFAULT -1, "visits" INTEGER NOT NULL DEFAULT 0);
CREATE TABLE "tags" ("id" INTEGER PRIMARY KEY, "tag" TEXT NOT NULL UNIQUE);
CREATE TABLE "bookmarks_tags" ("bookmark" INTEGER NOT NULL, "tag" INTEGER NOT NULL, PRIMARY KEY("bookmark", "tag"));
