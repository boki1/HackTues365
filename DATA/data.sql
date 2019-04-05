PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE Details (id TEXT, person TEXT, contact_name TEXT, contact_phone TEXT, birth_date TEXT, room INTEGER, enter_date TEXT);
CREATE TABLE Medicines (id TEXT, medicines TEXT, hours TEXT);
COMMIT;
