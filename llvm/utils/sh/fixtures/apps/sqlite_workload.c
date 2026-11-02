/* SH-CORPUS-11 development-only deterministic SQLite 3.46.1 smoke.
 * Newly authored workload, NOT the historical COMPAT-6 harness.
 */
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(sqlite3 *db, const char *where, int code) {
    fprintf(stderr, "%s failed: %d: %s\n", where, code, db ? sqlite3_errmsg(db) : "no db");
    if (db) sqlite3_close(db);
    return 1;
}

int main(void) {
    sqlite3 *db = NULL;
    sqlite3_stmt *ins = NULL;
    sqlite3_stmt *qry = NULL;
    char name[12];
    int rc, i, rows = 0;
    if (sqlite3_libversion_number() != 3046001) {
        fprintf(stderr, "wrong SQLite version: %s\n", sqlite3_libversion());
        return 2;
    }
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) return fail(db, "sqlite3_open", rc);
    rc = sqlite3_exec(db,
        "CREATE TABLE item(g INTEGER NOT NULL, n INTEGER NOT NULL, body TEXT NOT NULL);"
        "CREATE INDEX item_gn ON item(g,n);BEGIN IMMEDIATE;", NULL, NULL, NULL);
    if (rc != SQLITE_OK) return fail(db, "schema/begin", rc);
    rc = sqlite3_prepare_v2(db, "INSERT INTO item(g,n,body) VALUES(?1,?2,?3)", -1, &ins, NULL);
    if (rc != SQLITE_OK) return fail(db, "prepare insert", rc);
    for (i = 0; i < 61; ++i) {
        snprintf(name, sizeof(name), "v%03d", i);
        if (sqlite3_bind_int(ins, 1, i % 5) != SQLITE_OK ||
            sqlite3_bind_int(ins, 2, i * 17 - 207) != SQLITE_OK ||
            sqlite3_bind_text(ins, 3, name, -1, SQLITE_TRANSIENT) != SQLITE_OK) {
            sqlite3_finalize(ins);
            return fail(db, "bind", SQLITE_ERROR);
        }
        rc = sqlite3_step(ins);
        if (rc != SQLITE_DONE) {
            sqlite3_finalize(ins);
            return fail(db, "insert", rc);
        }
        sqlite3_reset(ins);
        sqlite3_clear_bindings(ins);
    }
    sqlite3_finalize(ins);
    rc = sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
    if (rc != SQLITE_OK) return fail(db, "commit", rc);
    rc = sqlite3_prepare_v2(db,
        "SELECT g, COUNT(*), SUM(n), SUM(n*n) FROM item "
        "WHERE n%3!=0 GROUP BY g ORDER BY g", -1, &qry, NULL);
    if (rc != SQLITE_OK) return fail(db, "prepare query", rc);
    puts("sqlite-new:3.46.1");
    while ((rc = sqlite3_step(qry)) == SQLITE_ROW) {
        printf("sqlite-new:%d:%d:%lld:%lld\n", sqlite3_column_int(qry, 0),
               sqlite3_column_int(qry, 1),
               (long long)sqlite3_column_int64(qry, 2),
               (long long)sqlite3_column_int64(qry, 3));
        ++rows;
    }
    sqlite3_finalize(qry);
    if (rc != SQLITE_DONE || rows != 5) return fail(db, "query", rc);
    printf("sqlite-new:changes:%d\n", sqlite3_total_changes(db));
    rc = sqlite3_close(db);
    if (rc != SQLITE_OK) return fail(NULL, "close", rc);
    return 0;
}
