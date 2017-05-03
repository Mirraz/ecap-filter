--PRAGMA foreign_keys = ON;
--BEGIN TRANSACTION;

CREATE TABLE "sites" (
	"domain" TEXT PRIMARY KEY NOT NULL,
	"categories" TEXT NOT NULL
);

CREATE TABLE "rules" (
	"category_id" INTEGER PRIMARY KEY NOT NULL,
	"allowed" INTEGER NOT NULL
);

--COMMIT;