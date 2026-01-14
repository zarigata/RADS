/*
 * RADS Constellation - Predictive Scaling Implementation
 * Phase 9 (Iguana): Nice to Have Features
 */

#include "predictive.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#define MAX_PREDICTORS 100

typedef struct {
    Predictor *predictors[MAX_PREDICTORS];
    int predictor_count;
    pthread_mutex_t lock;
    
    uint64_t total_predictions;
    uint64_t accurate_predictions;
    
    bool is_initialized;
} PredictiveState;

static PredictiveState g_state = {0};

int predictive_init(void) {
    if (g_state.is_initialized) {
        return 0;
    }
    
    memset(&g_state, 0, sizeof(PredictiveState));
    pthread_mutex_init(&g_state.lock, NULL);
    g_state.is_initialized = true;
    
    return 0;
}

int predictive_shutdown(void) {
    if (!g_state.is_initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    for (int i = 0; i < g_state.predictor_count; i++) {
        if (g_state.predictors[i]) {
            if (g_state.predictors[i]->training_data) {
                free(g_state.predictors[i]->training_data);
            }
            if (g_state.predictors[i]->coefficients) {
                free(g_state.predictors[i]->coefficients);
            }
            free(g_state.predictors[i]);
        }
    }
    
    g_state.predictor_count = 0;
    g_state.is_initialized = false;
    
    pthread_mutex_unlock(&g_state.lock);
    pthread_mutex_destroy(&g_state.lock);
    
    return 0;
}

bool predictive_is_running(void) {
    return g_state.is_initialized;
}

Predictor *predictive_create_predictor(const char *metric_name, PredictorConfig *config) {
    if (!g_state.is_initialized || !metric_name || !config) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    if (g_state.predictor_count >= MAX_PREDICTORS) {
        pthread_mutex_unlock(&g_state.lock);
        return NULL;
    }
    
    Predictor *predictor = calloc(1, sizeof(Predictor));
    if (!predictor) {
        pthread_mutex_unlock(&g_state.lock);
        return NULL;
    }
    
    strncpy(predictor->metric_name, metric_name, MAX_METRIC_NAME - 1);
    memcpy(&predictor->config, config, sizeof(PredictorConfig));
    
    predictor->training_capacity = MAX_TRAINING_SAMPLES;
    predictor->training_data = calloc(predictor->training_capacity, sizeof(MetricDataPoint));
    if (!predictor->training_data) {
        free(predictor);
        pthread_mutex_unlock(&g_state.lock);
        return NULL;
    }
    
    predictor->coefficient_count = 10;
    predictor->coefficients = calloc(predictor->coefficient_count, sizeof(double));
    if (!predictor->coefficients) {
        free(predictor->training_data);
        free(predictor);
        pthread_mutex_unlock(&g_state.lock);
        return NULL;
    }
    
    predictor->created_at = time(NULL);
    predictor->last_updated = predictor->created_at;
    predictor->is_trained = false;
    
    g_state.predictors[g_state.predictor_count++] = predictor;
    
    pthread_mutex_unlock(&g_state.lock);
    return predictor;
}

int predictive_destroy_predictor(const char *metric_name) {
    if (!g_state.is_initialized || !metric_name) {
        return -1;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    for (int i = 0; i < g_state.predictor_count; i++) {
        if (strcmp(g_state.predictors[i]->metric_name, metric_name) == 0) {
            if (g_state.predictors[i]->training_data) {
                free(g_state.predictors[i]->training_data);
            }
            if (g_state.predictors[i]->coefficients) {
                free(g_state.predictors[i]->coefficients);
            }
            free(g_state.predictors[i]);
            
            for (int j = i; j < g_state.predictor_count - 1; j++) {
                g_state.predictors[j] = g_state.predictors[j + 1];
            }
            g_state.predictor_count--;
            
            pthread_mutex_unlock(&g_state.lock);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_state.lock);
    return -1;
}

Predictor *predictive_get_predictor(const char *metric_name) {
    if (!g_state.is_initialized || !metric_name) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    for (int i = 0; i < g_state.predictor_count; i++) {
        if (strcmp(g_state.predictors[i]->metric_name, metric_name) == 0) {
            Predictor *predictor = g_state.predictors[i];
            pthread_mutex_unlock(&g_state.lock);
            return predictor;
        }
    }
    
    pthread_mutex_unlock(&g_state.lock);
    return NULL;
}

int predictive_add_training_sample(const char *metric_name, double value, time_t timestamp) {
    Predictor *predictor = predictive_get_predictor(metric_name);
    if (!predictor) {
        return -1;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    if (predictor->training_count < predictor->training_capacity) {
        predictor->training_data[predictor->training_count].value = value;
        predictor->training_data[predictor->training_count].timestamp = timestamp;
        predictor->training_count++;
    } else {
        memmove(&predictor->training_data[0], &predictor->training_data[1],
                (predictor->training_capacity - 1) * sizeof(MetricDataPoint));
        predictor->training_data[predictor->training_capacity - 1].value = value;
        predictor->training_data[predictor->training_capacity - 1].timestamp = timestamp;
    }
    
    predictor->last_updated = time(NULL);
    
    pthread_mutex_unlock(&g_state.lock);
    return 0;
}

void calculate_statistics(Predictor *predictor) {
    if (!predictor || predictor->training_count == 0) {
        return;
    }
    
    double sum = 0.0;
    predictor->min_value = predictor->training_data[0].value;
    predictor->max_value = predictor->training_data[0].value;
    
    for (int i = 0; i < predictor->training_count; i++) {
        double val = predictor->training_data[i].value;
        sum += val;
        if (val < predictor->min_value) predictor->min_value = val;
        if (val > predictor->max_value) predictor->max_value = val;
    }
    
    predictor->mean = sum / predictor->training_count;
    
    double variance = 0.0;
    for (int i = 0; i < predictor->training_count; i++) {
        double diff = predictor->training_data[i].value - predictor->mean;
        variance += diff * diff;
    }
    predictor->std_dev = sqrt(variance / predictor->training_count);
}

double predict_linear_regression(Predictor *predictor, int seconds_ahead) {
    if (!predictor || predictor->training_count < 2) {
        return predictor->mean;
    }
    
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_xx = 0.0;
    int n = predictor->training_count;
    
    for (int i = 0; i < n; i++) {
        double x = (double)i;
        double y = predictor->training_data[i].value;
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_xx += x * x;
    }
    
    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x * sum_x);
    double intercept = (sum_y - slope * sum_x) / n;
    
    predictor->coefficients[0] = slope;
    predictor->coefficients[1] = intercept;
    
    double future_x = n + (seconds_ahead / 60.0);
    return slope * future_x + intercept;
}

double predict_exponential_smoothing(Predictor *predictor, int seconds_ahead) {
    if (!predictor || predictor->training_count < 1) {
        return predictor->mean;
    }
    
    double alpha = predictor->config.smoothing_factor;
    if (alpha <= 0.0 || alpha > 1.0) {
        alpha = 0.3;
    }
    
    double smoothed = predictor->training_data[0].value;
    for (int i = 1; i < predictor->training_count; i++) {
        smoothed = alpha * predictor->training_data[i].value + (1 - alpha) * smoothed;
    }
    
    return smoothed;
}

double predict_seasonal(Predictor *predictor, int seconds_ahead) {
    if (!predictor || predictor->training_count < predictor->config.seasonal_period) {
        return predictor->mean;
    }
    
    int period = predictor->config.seasonal_period;
    if (period <= 0) {
        period = 3600;
    }
    
    int offset = seconds_ahead % period;
    int lookback_idx = predictor->training_count - (period - offset);
    
    if (lookback_idx >= 0 && lookback_idx < predictor->training_count) {
        return predictor->training_data[lookback_idx].value;
    }
    
    return predictor->mean;
}

double predict_ensemble(Predictor *predictor, int seconds_ahead) {
    double linear = predict_linear_regression(predictor, seconds_ahead);
    double exponential = predict_exponential_smoothing(predictor, seconds_ahead);
    double seasonal = predict_seasonal(predictor, seconds_ahead);
    
    return (linear * 0.4 + exponential * 0.3 + seasonal * 0.3);
}

int predictive_train(const char *metric_name) {
    Predictor *predictor = predictive_get_predictor(metric_name);
    if (!predictor) {
        return -1;
    }
    
    if (predictor->training_count < predictor->config.min_samples_required) {
        return -1;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    calculate_statistics(predictor);
    
    if (predictor->config.enable_pattern_detection) {
        detect_seasonal_patterns(predictor);
    }
    
    switch (predictor->config.model) {
        case PREDICT_MODEL_LINEAR_REGRESSION:
            predict_linear_regression(predictor, 0);
            break;
        case PREDICT_MODEL_EXPONENTIAL:
            predict_exponential_smoothing(predictor, 0);
            break;
        case PREDICT_MODEL_SEASONAL:
            predict_seasonal(predictor, 0);
            break;
        case PREDICT_MODEL_ENSEMBLE:
            predict_ensemble(predictor, 0);
            break;
        default:
            break;
    }
    
    predictor->is_trained = true;
    predictor->config.last_trained = time(NULL);
    
    pthread_mutex_unlock(&g_state.lock);
    return 0;
}

PredictionResult *predictive_predict(const char *metric_name, int seconds_ahead) {
    Predictor *predictor = predictive_get_predictor(metric_name);
    if (!predictor || !predictor->is_trained) {
        return NULL;
    }
    
    if (seconds_ahead < 0 || seconds_ahead > MAX_PREDICTION_WINDOW) {
        return NULL;
    }
    
    PredictionResult *result = calloc(1, sizeof(PredictionResult));
    if (!result) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    switch (predictor->config.model) {
        case PREDICT_MODEL_LINEAR_REGRESSION:
            result->predicted_value = predict_linear_regression(predictor, seconds_ahead);
            result->model_used = PREDICT_MODEL_LINEAR_REGRESSION;
            break;
        case PREDICT_MODEL_EXPONENTIAL:
            result->predicted_value = predict_exponential_smoothing(predictor, seconds_ahead);
            result->model_used = PREDICT_MODEL_EXPONENTIAL;
            break;
        case PREDICT_MODEL_SEASONAL:
            result->predicted_value = predict_seasonal(predictor, seconds_ahead);
            result->model_used = PREDICT_MODEL_SEASONAL;
            break;
        case PREDICT_MODEL_ENSEMBLE:
            result->predicted_value = predict_ensemble(predictor, seconds_ahead);
            result->model_used = PREDICT_MODEL_ENSEMBLE;
            break;
        default:
            result->predicted_value = predictor->mean;
            result->model_used = PREDICT_MODEL_LINEAR_REGRESSION;
    }
    
    result->confidence = 0.85;
    result->lower_bound = result->predicted_value - predictor->std_dev;
    result->upper_bound = result->predicted_value + predictor->std_dev;
    result->prediction_time = time(NULL) + seconds_ahead;
    
    snprintf(result->reasoning, sizeof(result->reasoning),
             "Predicted using %s model with %d samples, confidence %.2f",
             result->model_used == PREDICT_MODEL_LINEAR_REGRESSION ? "linear regression" :
             result->model_used == PREDICT_MODEL_EXPONENTIAL ? "exponential smoothing" :
             result->model_used == PREDICT_MODEL_SEASONAL ? "seasonal" : "ensemble",
             predictor->training_count, result->confidence);
    
    g_state.total_predictions++;
    
    pthread_mutex_unlock(&g_state.lock);
    return result;
}

void predictive_result_free(PredictionResult *result) {
    if (result) {
        free(result);
    }
}

void detect_seasonal_patterns(Predictor *predictor) {
    if (!predictor || predictor->training_count < 100) {
        return;
    }
    
    predictor->pattern_count = 0;
    
    int common_periods[] = {3600, 86400, 604800};
    int period_count = 3;
    
    for (int p = 0; p < period_count && predictor->pattern_count < MAX_PATTERN_COUNT; p++) {
        int period = common_periods[p];
        if (predictor->training_count < period / 60) {
            continue;
        }
        
        TimeSeriesPattern *pattern = &predictor->patterns[predictor->pattern_count];
        pattern->period_seconds = period;
        pattern->amplitude = predictor->std_dev;
        pattern->phase = 0.0;
        pattern->confidence = 0.7;
        
        snprintf(pattern->name, sizeof(pattern->name), 
                 period == 3600 ? "hourly" : 
                 period == 86400 ? "daily" : "weekly");
        
        predictor->pattern_count++;
    }
}

PredictorStats predictive_get_stats(void) {
    PredictorStats stats = {0};
    
    if (!g_state.is_initialized) {
        return stats;
    }
    
    pthread_mutex_lock(&g_state.lock);
    
    stats.total_predictors = g_state.predictor_count;
    stats.total_predictions = g_state.total_predictions;
    stats.accurate_predictions = g_state.accurate_predictions;
    
    for (int i = 0; i < g_state.predictor_count; i++) {
        if (g_state.predictors[i]->config.enabled) {
            stats.active_predictors++;
        }
    }
    
    if (stats.total_predictions > 0) {
        stats.average_accuracy = (double)stats.accurate_predictions / stats.total_predictions;
    }
    
    pthread_mutex_unlock(&g_state.lock);
    return stats;
}

double predictive_get_trend(const char *metric_name) {
    Predictor *predictor = predictive_get_predictor(metric_name);
    if (!predictor || !predictor->is_trained) {
        return 0.0;
    }
    
    return predictor->coefficients[0];
}

bool predictive_needs_retraining(Predictor *predictor) {
    if (!predictor) {
        return false;
    }
    
    time_t now = time(NULL);
    return (now - predictor->config.last_trained) > predictor->config.retrain_interval_seconds;
}

int predictive_train_all(void) {
    if (!g_state.is_initialized) {
        return -1;
    }
    
    int trained = 0;
    for (int i = 0; i < g_state.predictor_count; i++) {
        if (predictive_train(g_state.predictors[i]->metric_name) == 0) {
            trained++;
        }
    }
    
    return trained;
}
