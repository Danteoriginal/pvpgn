#ifdef WITH_SQL_PGSQL

#include "common/setup_before.h"
#include <libpq-fe.h>
#include <stdlib.h>
#include "common/eventlog.h"
#include "storage_sql.h"
#include "sql_pgsql.h"
#include "common/setup_after.h"

static int sql_pgsql_init(const char *, const char *, const char *, const char *, const char *, const char *);
static int sql_pgsql_close(void);
static t_sql_res * sql_pgsql_query_res(const char *);
static int sql_pgsql_query(const char *);
static t_sql_row * sql_pgsql_fetch_row(t_sql_res *);
static void sql_pgsql_free_result(t_sql_res *);
static unsigned int sql_pgsql_num_rows(t_sql_res *);
static unsigned int sql_pgsql_num_fields(t_sql_res *);
static t_sql_field * sql_pgsql_fetch_fields(t_sql_res *);
static int sql_pgsql_free_fields(t_sql_field *);
static void sql_pgsql_escape_string(char *, const char *, int);

t_sql_engine sql_pgsql = {
    sql_pgsql_init,
    sql_pgsql_close,
    sql_pgsql_query_res,
    sql_pgsql_query,
    sql_pgsql_fetch_row,
    sql_pgsql_free_result,
    sql_pgsql_num_rows,
    sql_pgsql_num_fields,
    sql_pgsql_fetch_fields,
    sql_pgsql_free_fields,
    sql_pgsql_escape_string
};

static PGconn *pgsql = NULL;

typedef struct {
    int crow;
    char ** rowbuf;
    PGresult *pgres;
} t_pgsql_res;

static int sql_pgsql_init(const char *host, const char *port, const char *socket, const char *name, const char *user, const char *pass)
{
    const char *tmphost;

    if (name == NULL || user == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "got NULL parameter");
        return -1;
    }

    tmphost = host != NULL ? host : socket;

    if ((pgsql = PQsetdbLogin(host, port, NULL, NULL, name, user, pass)) == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "not enougn memory for new pgsql connection");
        return -1;
    }

    if (PQstatus(pgsql) != CONNECTION_OK) {
        eventlog(eventlog_level_error, __FUNCTION__, "error connecting to database");
	PQfinish(pgsql);
	pgsql = NULL;
        return -1;
    }

    return 0;
}

static int sql_pgsql_close(void)
{
    if (pgsql) {
	PQfinish(pgsql);
	pgsql = NULL;
    }

    return 0;
}

static t_sql_res * sql_pgsql_query_res(const char * query)
{
    t_pgsql_res *res;
    PGresult *pgres;

    if (pgsql == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "pgsql driver not initilized");
        return NULL;
    }

    if (query == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "got NULL query");
        return NULL;
    }

    if ((pgres = PQexec(pgsql, query)) == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "not enough memory for query (%s)", query);
	return NULL;
    }

    if (PQresultStatus(pgres) != PGRES_TUPLES_OK) {
/*        eventlog(eventlog_level_debug, __FUNCTION__, "got error from query (%s)", query); */
	PQclear(pgres);
	return NULL;
    }

    if ((res = (t_pgsql_res *)malloc(sizeof(t_pgsql_res))) == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "not enough memory for pgsql result");
	PQclear(pgres);
	return NULL;
    }

    if ((res->rowbuf = malloc(sizeof(char *) * PQnfields(pgres))) == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "not enough memory for row buffer");
	free((void*)res);
	PQclear(pgres);
	return NULL;
    }

    res->pgres = pgres;
    res->crow = 0;

/*    eventlog(eventlog_level_debug, __FUNCTION__, "res: %p res->rowbuf: %p res->crow: %d res->pgres: %p", res, res->rowbuf, res->crow, res->pgres); */
    return res;
}

static int sql_pgsql_query(const char * query)
{
    PGresult *pgres;
    int res;

    if (pgsql == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "pgsql driver not initilized");
        return -1;
    }

    if (query == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "got NULL query");
        return -1;
    }

    if ((pgres = PQexec(pgsql, query)) == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "not enough memory for result");
        return -1;
    }

    res = PQresultStatus(pgres) == PGRES_COMMAND_OK ? 0 : -1;
    PQclear(pgres);

    return res;
}

static t_sql_row * sql_pgsql_fetch_row(t_sql_res *result)
{
    int nofields, i;
    t_pgsql_res *res = (t_pgsql_res *) result;

    if (res == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return NULL;
    }

    if (res->crow < 0) {
	eventlog(eventlog_level_error, __FUNCTION__, "got called without a proper res query");
	return NULL;
    }

    if (res->crow >= PQntuples(res->pgres)) return NULL; /* end of result */

    nofields = PQnfields(res->pgres);
    for(i = 0; i < nofields; i++) {
	res->rowbuf[i] = PQgetvalue(res->pgres, res->crow, i);
	/* the next line emulates the mysql way where NULL containing fields return NULL */
	if (res->rowbuf[i] && res->rowbuf[i][0] == '\0') res->rowbuf[i] = NULL;
    }

    res->crow++;

/*    eventlog(eventlog_level_debug, __FUNCTION__, "res: %p res->rowbuf: %p res->crow: %d res->pgres: %p", res, res->rowbuf, res->crow, res->pgres); */
    return res->rowbuf;
}

static void sql_pgsql_free_result(t_sql_res *result)
{
    t_pgsql_res *res = (t_pgsql_res *) result;

    if (res == NULL) return;
/*    eventlog(eventlog_level_debug, __FUNCTION__, "res: %p res->rowbuf: %p res->crow: %d res->pgres: %p", res, res->rowbuf, res->crow, res->pgres); */

    if (res->pgres) PQclear(res->pgres);
    if (res->rowbuf) free((void*)res->rowbuf);
    free((void*)res);
}

static unsigned int sql_pgsql_num_rows(t_sql_res *result)
{
    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return 0;
    }

    return PQntuples(((t_pgsql_res *)result)->pgres);
}

static unsigned int sql_pgsql_num_fields(t_sql_res *result)
{
    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return 0;
    }

    return PQnfields(((t_pgsql_res *)result)->pgres);
}

static t_sql_field * sql_pgsql_fetch_fields(t_sql_res *result)
{
    t_pgsql_res *res = (t_pgsql_res *) result;
    unsigned fieldno, i;
    t_sql_field *rfields;

    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return NULL;
    }

    fieldno = PQnfields(res->pgres);

    if ((rfields = malloc(sizeof(t_sql_field) * (fieldno + 1))) == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "not enough memory for field list");
	return NULL;
    }

    for(i = 0; i < fieldno; i++)
	rfields[i] = PQfname(res->pgres, i);
    rfields[i] = NULL;

    return rfields;
}

static int sql_pgsql_free_fields(t_sql_field *fields)
{
    if (fields == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL fields");
	return -1;
    }

    free((void*)fields);
    return 0; /* PQclear() should free the rest properly */
}

static void sql_pgsql_escape_string(char *escape, const char *from, int len)
{
    PQescapeString(escape, from, len);
}

#endif /* WITH_SQL_PGSQL */
