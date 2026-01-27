#include "stdlib_graphql.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define GQL_MAX_FIELDS 128
#define GQL_MAX_TYPES 32
#define GQL_QUERY_BUFFER 4096

typedef struct {
    int fd;
    GraphQLSchema* schema;
    bool running;
} GraphQLServerImpl;

static char* trim_whitespace(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;

    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

GraphQLSchema* graphql_schema_create(void) {
    GraphQLSchema* schema = (GraphQLSchema*)calloc(1, sizeof(GraphQLSchema));
    schema->types = (GraphQLTypeDef**)malloc(GQL_MAX_TYPES * sizeof(GraphQLTypeDef*));
    schema->type_count = 0;
    schema->queries = (GraphQLField*)calloc(GQL_MAX_FIELDS, sizeof(GraphQLField));
    schema->mutations = (GraphQLField*)calloc(GQL_MAX_FIELDS, sizeof(GraphQLField));
    schema->subscriptions = (GraphQLField*)calloc(GQL_MAX_FIELDS, sizeof(GraphQLField));
    schema->user_data = NULL;
    return schema;
}

void graphql_schema_free(GraphQLSchema* schema) {
    if (!schema) return;

    for (int i = 0; i < schema->type_count; i++) {
        if (schema->types[i]) {
            free(schema->types[i]->name);
            for (int j = 0; j < schema->types[i]->field_count; j++) {
                free(schema->types[i]->fields[j].name);
                if (schema->types[i]->fields[j].args) {
                    free(schema->types[i]->fields[j].args);
                }
            }
            free(schema->types[i]->fields);
            free(schema->types[i]);
        }
    }

    free(schema->types);
    free(schema->queries);
    free(schema->mutations);
    free(schema->subscriptions);
    free(schema);
}

int graphql_schema_add_type(GraphQLSchema* schema, const char* name, GraphQLField* fields, int field_count) {
    if (schema->type_count >= GQL_MAX_TYPES) return -1;

    GraphQLTypeDef* type = (GraphQLTypeDef*)malloc(sizeof(GraphQLTypeDef));
    type->name = strdup(name);
    type->fields = (GraphQLField*)malloc(field_count * sizeof(GraphQLField));
    type->field_count = field_count;

    for (int i = 0; i < field_count; i++) {
        type->fields[i].name = strdup(fields[i].name);
        type->fields[i].args = fields[i].args ? (GraphQLArgument*)malloc(fields[i].arg_count * sizeof(GraphQLArgument)) : NULL;
        type->fields[i].arg_count = fields[i].arg_count;
        type->fields[i].value = NULL;

        for (int j = 0; j < fields[i].arg_count; j++) {
            type->fields[i].args[j].name = strdup(fields[i].args[j].name);
            type->fields[i].args[j].value = fields[i].args[j].value;
        }
    }

    schema->types[schema->type_count++] = type;
    return 0;
}

int graphql_schema_add_query(GraphQLSchema* schema, GraphQLField* field) {
    for (int i = 0; i < GQL_MAX_FIELDS; i++) {
        if (!schema->queries[i].name) {
            schema->queries[i].name = strdup(field->name);
            schema->queries[i].args = field->args ? (GraphQLArgument*)malloc(field->arg_count * sizeof(GraphQLArgument)) : NULL;
            schema->queries[i].arg_count = field->arg_count;
            schema->queries[i].value = field->value;
            return 0;
        }
    }
    return -1;
}

int graphql_schema_add_mutation(GraphQLSchema* schema, GraphQLField* field) {
    for (int i = 0; i < GQL_MAX_FIELDS; i++) {
        if (!schema->mutations[i].name) {
            schema->mutations[i].name = strdup(field->name);
            schema->mutations[i].args = field->args ? (GraphQLArgument*)malloc(field->arg_count * sizeof(GraphQLArgument)) : NULL;
            schema->mutations[i].arg_count = field->arg_count;
            schema->mutations[i].value = field->value;
            return 0;
        }
    }
    return -1;
}

int graphql_schema_add_subscription(GraphQLSchema* schema, GraphQLField* field) {
    for (int i = 0; i < GQL_MAX_FIELDS; i++) {
        if (!schema->subscriptions[i].name) {
            schema->subscriptions[i].name = strdup(field->name);
            schema->subscriptions[i].args = field->args ? (GraphQLArgument*)malloc(field->arg_count * sizeof(GraphQLArgument)) : NULL;
            schema->subscriptions[i].arg_count = field->arg_count;
            schema->subscriptions[i].value = field->value;
            return 0;
        }
    }
    return -1;
}

void graphql_field_set_resolver(GraphQLField* field, GraphQLResolver resolver, void* user_data) {
    if (!field) return;
    field->value = resolver;
}

GraphQLValue* graphql_value_string(const char* str) {
    GraphQLValue* v = (GraphQLValue*)malloc(sizeof(GraphQLValue));
    v->type = GQL_TYPE_SCALAR;
    v->is_null = false;
    v->as.string = strdup(str);
    return v;
}

GraphQLValue* graphql_value_number(double num) {
    GraphQLValue* v = (GraphQLValue*)malloc(sizeof(GraphQLValue));
    v->type = GQL_TYPE_SCALAR;
    v->is_null = false;
    v->as.number = num;
    return v;
}

GraphQLValue* graphql_value_bool(bool b) {
    GraphQLValue* v = (GraphQLValue*)malloc(sizeof(GraphQLValue));
    v->type = GQL_TYPE_SCALAR;
    v->is_null = false;
    v->as.boolean = b;
    return v;
}

static GraphQLValue _null_value = { .type = GQL_TYPE_SCALAR, .is_null = true };

GraphQLValue* graphql_value_null(void) {
    return &_null_value;
}

GraphQLValue* graphql_value_list(GraphQLValue** items, int count) {
    GraphQLValue* v = (GraphQLValue*)malloc(sizeof(GraphQLValue));
    v->type = GQL_TYPE_LIST;
    v->is_null = false;
    v->as.list = items;
    return v;
}

GraphQLValue* graphql_value_object(GraphQLField** fields, int count) {
    GraphQLValue* v = (GraphQLValue*)malloc(sizeof(GraphQLValue));
    v->type = GQL_TYPE_OBJECT;
    v->is_null = false;
    v->as.object.fields = (GraphQLField*)malloc(count * sizeof(GraphQLField));
    v->as.object.count = count;
    return v;
}

void graphql_value_free(GraphQLValue* value) {
    if (!value) return;

    if (!value->is_null) {
        switch (value->type) {
            case GQL_TYPE_SCALAR:
                if (value->as.string) free(value->as.string);
                break;
            case GQL_TYPE_LIST:
                if (value->as.list) {
                    for (int i = 0; i < 10; i++) {
                        if (value->as.list[i]) graphql_value_free(value->as.list[i]);
                    }
                    free(value->as.list);
                }
                break;
            case GQL_TYPE_OBJECT:
                if (value->as.object.fields) free(value->as.object.fields);
                break;
            default:
                break;
        }
    }

    free(value);
}

char* graphql_value_to_json(GraphQLValue* value) {
    if (!value) return strdup("null");

    if (value->is_null) return strdup("null");

    char buffer[1024];

    switch (value->type) {
        case GQL_TYPE_SCALAR:
            if (value->as.string) {
                snprintf(buffer, sizeof(buffer), "\"%s\"", value->as.string);
            } else if (value->as.boolean) {
                snprintf(buffer, sizeof(buffer), "%s", value->as.boolean ? "true" : "false");
            } else {
                snprintf(buffer, sizeof(buffer), "%g", value->as.number);
            }
            break;

        case GQL_TYPE_LIST:
            snprintf(buffer, sizeof(buffer), "[...]");
            break;

        case GQL_TYPE_OBJECT:
            snprintf(buffer, sizeof(buffer), "{...}");
            break;

        default:
            snprintf(buffer, sizeof(buffer), "null");
            break;
    }

    return strdup(buffer);
}

static GraphQLField* find_field(GraphQLField* fields, int field_count, const char* name) {
    for (int i = 0; i < field_count; i++) {
        if (fields[i].name && strcmp(fields[i].name, name) == 0) {
            return &fields[i];
        }
    }
    return NULL;
}

static GraphQLValue* resolve_field(GraphQLContext* ctx, GraphQLField* field) {
    if (field->value) {
        GraphQLResolver resolver = (GraphQLResolver)field->value;
        return resolver(ctx, field, ctx->schema->user_data);
    }

    return graphql_value_null();
}

GraphQLQuery* graphql_parse(const char* query, char** error) {
    if (!query || !query[0]) {
        if (error) *error = strdup("Empty query");
        return NULL;
    }

    GraphQLQuery* q = (GraphQLQuery*)calloc(1, sizeof(GraphQLQuery));

    char* operation_start = strstr(query, "query");
    char* mutation_start = strstr(query, "mutation");

    if (operation_start) {
        q->operation = GQL_OP_QUERY;
        operation_start += 5;
    } else if (mutation_start) {
        q->operation = GQL_OP_MUTATION;
        operation_start = mutation_start + 8;
    } else {
        q->operation = GQL_OP_QUERY;
        operation_start = (char*)query;
    }

    char* brace_start = strchr(operation_start, '{');
    if (!brace_start) {
        if (error) *error = strdup("Invalid GraphQL syntax");
        free(q);
        return NULL;
    }

    brace_start++;

    char* brace_end = strchr(brace_start, '}');
    if (!brace_end) {
        if (error) *error = strdup("Unclosed brace");
        free(q);
        return NULL;
    }

    int field_count = 0;
    char* current = brace_start;

    while (current < brace_end) {
        if (*current && !isspace((unsigned char)*current)) {
            if (field_count > 0) field_count++;
        }
        current++;
    }
    if (brace_end > brace_start) field_count++;

    q->selection_set = (GraphQLField*)malloc(field_count * sizeof(GraphQLField));
    q->selection_count = 0;
    q->query = strdup(query);
    q->variables = NULL;
    q->variable_count = 0;
    q->result = NULL;
    q->error = NULL;

    return q;
}

void graphql_query_free(GraphQLQuery* query) {
    if (!query) return;

    if (query->query) free(query->query);
    if (query->selection_set) free(query->selection_set);
    if (query->result) graphql_value_free(query->result);
    if (query->error) free(query->error);
    if (query->variables) {
        for (int i = 0; i < query->variable_count; i++) {
            if (query->variables[i].name) free(query->variables[i].name);
            if (query->variables[i].value) free(query->variables[i].value);
        }
        free(query->variables);
    }

    free(query);
}

GraphQLContext* graphql_execute(GraphQLSchema* schema, GraphQLQuery* query, GraphQLVariable* variables) {
    GraphQLContext* ctx = (GraphQLContext*)calloc(1, sizeof(GraphQLContext));
    ctx->query = query;
    ctx->variables = variables;
    ctx->variable_count = variables ? 1 : 0;
    ctx->schema = schema;
    ctx->result = NULL;
    ctx->error = NULL;

    GraphQLField* operation_fields = NULL;
    int field_count = 0;

    switch (query->operation) {
        case GQL_OP_QUERY:
            operation_fields = schema->queries;
            field_count = GQL_MAX_FIELDS;
            break;
        case GQL_OP_MUTATION:
            operation_fields = schema->mutations;
            field_count = GQL_MAX_FIELDS;
            break;
        case GQL_OP_SUBSCRIPTION:
            operation_fields = schema->subscriptions;
            field_count = GQL_MAX_FIELDS;
            break;
    }

    GraphQLValue** results = (GraphQLValue**)malloc(query->selection_count * sizeof(GraphQLValue*));
    int result_count = 0;

    for (int i = 0; i < query->selection_count; i++) {
        GraphQLField* field = find_field(operation_fields, field_count, query->selection_set[i].name);

        if (field) {
            GraphQLValue* resolved = resolve_field(ctx, field);
            if (resolved) {
                results[result_count++] = resolved;
            }
        }
    }

    if (result_count == 1) {
        ctx->result = results[0];
    } else if (result_count > 1) {
        ctx->result = graphql_value_list(results, result_count);
    } else {
        ctx->result = graphql_value_null();
    }

    return ctx;
}

void graphql_context_free(GraphQLContext* ctx) {
    if (!ctx) return;
    if (ctx->result && ctx->result != graphql_value_null()) {
        graphql_value_free(ctx->result);
    }
    if (ctx->error) free(ctx->error);
    free(ctx);
}

struct GraphQLServer* graphql_server_create(GraphQLSchema* schema, int port) {
    GraphQLServerImpl* server = (GraphQLServerImpl*)calloc(1, sizeof(GraphQLServerImpl));
    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    server->schema = schema;
    server->running = false;

    if (server->fd < 0) {
        perror("socket");
        free(server);
        return NULL;
    }

    int opt = 1;
    setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server->fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server->fd);
        free(server);
        return NULL;
    }

    if (listen(server->fd, 10) < 0) {
        perror("listen");
        close(server->fd);
        free(server);
        return NULL;
    }

    server->running = true;
    printf("GraphQL server listening on port %d\n", port);
    return (struct GraphQLServer*)server;
}

void graphql_server_free(struct GraphQLServer* server) {
    if (!server) return;

    GraphQLServerImpl* impl = (GraphQLServerImpl*)server;

    if (impl->fd >= 0) {
        close(impl->fd);
    }

    free(impl);
}

int graphql_server_listen(struct GraphQLServer* server) {
    GraphQLServerImpl* impl = (GraphQLServerImpl*)server;

    while (impl->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(impl->fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd >= 0) {
            char buffer[4096];
            ssize_t len = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

            if (len > 0) {
                buffer[len] = '\0';

                char* error = NULL;
                GraphQLQuery* query = graphql_parse(buffer, &error);

                if (query) {
                    GraphQLContext* ctx = graphql_execute(impl->schema, query, NULL);

                    char* json_result = graphql_value_to_json(ctx->result);

                    char response[8192];
                    snprintf(response, sizeof(response),
                        "{\n"
                        "  \"data\": %s%s%s\n"
                        "}\n",
                        json_result ? json_result : "null",
                        ctx->error ? ",\n  \"errors\": [{" : "",
                        ctx->error ? ctx->error : "");

                    send(client_fd, response, strlen(response), 0);

                    free(json_result);
                    graphql_context_free(ctx);
                    graphql_query_free(query);
                }

                close(client_fd);
            }
        }
    }

    return 0;
}

void graphql_server_stop(struct GraphQLServer* server) {
    if (!server) return;

    GraphQLServerImpl* impl = (GraphQLServerImpl*)server;
    impl->running = false;
}
