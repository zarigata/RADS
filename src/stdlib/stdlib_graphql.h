#ifndef RADS_GRAPHQL_H
#define RADS_GRAPHQL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    GQL_TYPE_SCALAR,
    GQL_TYPE_OBJECT,
    GQL_TYPE_LIST,
    GQL_TYPE_NON_NULL
} GraphQLType;

typedef enum {
    GQL_OP_QUERY,
    GQL_OP_MUTATION,
    GQL_OP_SUBSCRIPTION
} GraphQLOperation;

typedef struct GraphQLValue GraphQLValue;

typedef struct {
    char* key;
    char* value;
} GraphQLVariable;

typedef struct {
    char* name;
    GraphQLValue* value;
} GraphQLArgument;

typedef struct GraphQLValue GraphQLValue;

typedef struct GraphQLField GraphQLField;

typedef struct GraphQLSchema GraphQLSchema;

typedef struct GraphQLContext GraphQLContext;

typedef GraphQLValue* (*GraphQLResolver)(GraphQLContext* ctx, GraphQLField* field, void* user_data);

struct GraphQLField {
    char* name;
    GraphQLArgument* args;
    int arg_count;
    GraphQLValue* value;
    GraphQLResolver resolver;
};

typedef struct {
    char* name;
    GraphQLField* fields;
    int field_count;
} GraphQLTypeDef;

typedef struct {
    char* name;
    GraphQLOperation operation;
    GraphQLField* selection_set;
    int selection_count;
    char* query;
    GraphQLVariable* variables;
    int variable_count;
    GraphQLValue* result;
    char* error;
} GraphQLQuery;

typedef union {
    char* string;
    double number;
    bool boolean;
    GraphQLValue** list;
    struct {
        GraphQLField* fields;
        int count;
    } object;
} GraphQLValueUnion;

struct GraphQLValue {
    GraphQLType type;
    bool is_null;
    GraphQLValueUnion as;
};

struct GraphQLSchema {
    GraphQLTypeDef** types;
    int type_count;
    GraphQLField* queries;
    GraphQLField* mutations;
    GraphQLField* subscriptions;
    void* user_data;
};

struct GraphQLContext {
    GraphQLQuery* query;
    GraphQLVariable* variables;
    int variable_count;
    GraphQLValue* result;
    char* error;
    GraphQLSchema* schema;
};

typedef struct GraphQLServer GraphQLServer;

GraphQLSchema* graphql_schema_create(void);
void graphql_schema_free(GraphQLSchema* schema);

int graphql_schema_add_type(GraphQLSchema* schema, const char* name, GraphQLField* fields, int field_count);
int graphql_schema_add_query(GraphQLSchema* schema, GraphQLField* field);
int graphql_schema_add_mutation(GraphQLSchema* schema, GraphQLField* field);
int graphql_schema_add_subscription(GraphQLSchema* schema, GraphQLField* field);

void graphql_field_set_resolver(GraphQLField* field, GraphQLResolver resolver, void* user_data);

GraphQLValue* graphql_value_string(const char* str);
GraphQLValue* graphql_value_number(double num);
GraphQLValue* graphql_value_bool(bool b);
GraphQLValue* graphql_value_null(void);
GraphQLValue* graphql_value_list(GraphQLValue** items, int count);
GraphQLValue* graphql_value_object(GraphQLField** fields, int count);
void graphql_value_free(GraphQLValue* value);
char* graphql_value_to_json(GraphQLValue* value);

GraphQLQuery* graphql_parse(const char* query, char** error);
void graphql_query_free(GraphQLQuery* query);

GraphQLContext* graphql_execute(GraphQLSchema* schema, GraphQLQuery* query, GraphQLVariable* variables);
void graphql_context_free(GraphQLContext* ctx);

struct GraphQLServer* graphql_server_create(GraphQLSchema* schema, int port);
void graphql_server_free(struct GraphQLServer* server);
int graphql_server_listen(struct GraphQLServer* server);
void graphql_server_stop(struct GraphQLServer* server);

#endif
