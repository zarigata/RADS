/*
 * RADS Constellation - Predictive Scaling
 * Phase 9 (Iguana): Nice to Have Features
 *
 * Machine learning-based load prediction for proactive scaling
 * Uses time-series analysis and pattern recognition
 */

#ifndef CONSTELLATION_PREDICTIVE_H
#define CONSTELLATION_PREDICTIVE_H

#include "metrics.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * ============================================================================
 * PREDICTIVE SCALING
 * ============================================================================
 */

#define MAX_PREDICTION_WINDOW 3600      // 1 hour ahead
#define MAX_TRAINING_SAMPLES 10000      // Historical data points
#define MAX_PATTERN_COUNT 100           // Detected patterns

// Prediction model types
typedef enum {
    PREDICT_MODEL_LINEAR_REGRESSION = 0,  // Simple linear trend
    PREDICT_MODEL_EXPONENTIAL = 1,        // Exponential smoothing
    PREDICT_MODEL_ARIMA = 2,              // AutoRegressive Integrated Moving Average
    PREDICT_MODEL_SEASONAL = 3,           // Seasonal decomposition
    PREDICT_MODEL_ENSEMBLE = 4            // Combination of models
} PredictionModel;

// Time series pattern
typedef struct {
    char name[128];
    double period_seconds;        // Pattern period (e.g., 86400 for daily)
    double amplitude;             // Pattern strength
    double phase;                 // Phase offset
    double confidence;            // Confidence score (0-1)
} TimeSeriesPattern;

// Prediction result
typedef struct {
    double predicted_value;       // Predicted metric value
    double confidence;            // Confidence level (0-1)
    double lower_bound;           // Lower confidence interval
    double upper_bound;           // Upper confidence interval
    time_t prediction_time;       // When this prediction is for
    PredictionModel model_used;   // Which model was used
    char reasoning[512];          // Human-readable explanation
} PredictionResult;

// Predictor configuration
typedef struct {
    char metric_name[MAX_METRIC_NAME];
    PredictionModel model;
    
    // Training parameters
    int training_window_seconds;  // How far back to look
    int min_samples_required;     // Minimum data points needed
    double confidence_threshold;  // Minimum confidence to use prediction
    
    // Model-specific parameters
    double learning_rate;         // For adaptive models
    double smoothing_factor;      // For exponential smoothing
    int seasonal_period;          // For seasonal models (seconds)
    
    // Pattern detection
    bool enable_pattern_detection;
    int max_patterns;
    
    // Update frequency
    int retrain_interval_seconds;
    time_t last_trained;
    
    bool enabled;
} PredictorConfig;

// Predictor state
typedef struct {
    char metric_name[MAX_METRIC_NAME];
    PredictorConfig config;
    
    // Training data
    MetricDataPoint *training_data;
    int training_count;
    int training_capacity;
    
    // Detected patterns
    TimeSeriesPattern patterns[MAX_PATTERN_COUNT];
    int pattern_count;
    
    // Model coefficients (model-specific)
    double *coefficients;
    int coefficient_count;
    
    // Statistics
    double mean;
    double std_dev;
    double min_value;
    double max_value;
    
    // Performance metrics
    double mae;                   // Mean Absolute Error
    double rmse;                  // Root Mean Square Error
    double mape;                  // Mean Absolute Percentage Error
    
    time_t created_at;
    time_t last_updated;
    bool is_trained;
} Predictor;

// Predictor statistics
typedef struct {
    int total_predictors;
    int active_predictors;
    uint64_t total_predictions;
    uint64_t accurate_predictions;  // Within confidence interval
    double average_accuracy;
} PredictorStats;

/*
 * ============================================================================
 * LIFECYCLE
 * ============================================================================
 */

int predictive_init(void);
int predictive_shutdown(void);
bool predictive_is_running(void);

/*
 * ============================================================================
 * PREDICTOR MANAGEMENT
 * ============================================================================
 */

Predictor *predictive_create_predictor(
    const char *metric_name,
    PredictorConfig *config
);

int predictive_destroy_predictor(const char *metric_name);
Predictor *predictive_get_predictor(const char *metric_name);
Predictor **predictive_list_predictors(int *count);

/*
 * ============================================================================
 * TRAINING
 * ============================================================================
 */

int predictive_train(const char *metric_name);
int predictive_train_all(void);
bool predictive_needs_retraining(Predictor *predictor);

int predictive_add_training_sample(
    const char *metric_name,
    double value,
    time_t timestamp
);

/*
 * ============================================================================
 * PREDICTION
 * ============================================================================
 */

PredictionResult *predictive_predict(
    const char *metric_name,
    int seconds_ahead
);

PredictionResult **predictive_predict_range(
    const char *metric_name,
    int start_seconds,
    int end_seconds,
    int step_seconds,
    int *count
);

void predictive_result_free(PredictionResult *result);

/*
 * ============================================================================
 * PATTERN DETECTION
 * ============================================================================
 */

int predictive_detect_patterns(const char *metric_name);
TimeSeriesPattern *predictive_get_patterns(const char *metric_name, int *count);
bool predictive_has_seasonal_pattern(const char *metric_name);
double predictive_get_trend(const char *metric_name);

/*
 * ============================================================================
 * MODEL EVALUATION
 * ============================================================================
 */

double predictive_evaluate_accuracy(const char *metric_name);
double predictive_cross_validate(const char *metric_name, int folds);
int predictive_compare_models(
    const char *metric_name,
    PredictionModel *models,
    int model_count,
    PredictionModel *best_model
);

/*
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

PredictorStats predictive_get_stats(void);
double predictive_get_mae(const char *metric_name);
double predictive_get_rmse(const char *metric_name);
double predictive_get_mape(const char *metric_name);

/*
 * ============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================
 */

double predict_linear_regression(Predictor *predictor, int seconds_ahead);
double predict_exponential_smoothing(Predictor *predictor, int seconds_ahead);
double predict_arima(Predictor *predictor, int seconds_ahead);
double predict_seasonal(Predictor *predictor, int seconds_ahead);
double predict_ensemble(Predictor *predictor, int seconds_ahead);

void calculate_statistics(Predictor *predictor);
void detect_seasonal_patterns(Predictor *predictor);
void update_model_coefficients(Predictor *predictor);

#endif // CONSTELLATION_PREDICTIVE_H
