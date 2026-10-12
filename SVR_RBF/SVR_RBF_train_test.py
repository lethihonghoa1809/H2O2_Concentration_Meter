import pandas as pd
import numpy as np

from sklearn.svm import SVR
from sklearn.pipeline import make_pipeline
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import KFold, cross_val_predict
from sklearn.metrics import (
    mean_absolute_error,
    mean_squared_error,
    r2_score,
    max_error
)

# ============================================================
# 1. CAU HINH
# ============================================================

# Dat hai file CSV cung thu muc voi file Python.
TRAIN_FILE = "Tongket.csv"
TEST_FILE = "solieudonongdo.csv"

# Duong chuan Linear Regression: A = B1*C + B0
B0 = 0.09388
B1 = 0.00625

# Tham so SVR-RBF
SVR_C = 10
SVR_EPSILON = 0.1

# Cross Validation tren tap train
N_SPLITS = 5
RANDOM_STATE = 42

# File dau ra
OUTPUT_CSV = "Ket_qua_Linear_SVR_RBF.csv"
OUTPUT_TXT = "Thong_so_SVR_RBF.txt"
OUTPUT_H = "SVR_RBF_model.h"


# ============================================================
# 2. CAC HAM HO TRO
# ============================================================

def read_csv_checked(path):
    """Doc CSV va loai bo khoang trang trong ten cot."""
    try:
        df = pd.read_csv(path)
    except FileNotFoundError:
        raise FileNotFoundError(
            f"Khong tim thay file '{path}'. "
            "Hay dat file CSV cung thu muc voi file Python."
        )
    df.columns = df.columns.astype(str).str.strip()
    return df


def calculate_metrics(y_true, y_pred):
    """Tinh cac chi so danh gia."""
    y_true = np.asarray(y_true, dtype=float)
    y_pred = np.asarray(y_pred, dtype=float)

    return {
        "MAE": mean_absolute_error(y_true, y_pred),
        "RMSE": np.sqrt(mean_squared_error(y_true, y_pred)),
        "R2": r2_score(y_true, y_pred),
        "MaxError": max_error(y_true, y_pred)
    }


def print_metrics(title, result):
    print("\n" + "=" * 78)
    print(title)
    print("=" * 78)
    for name, value in result.items():
        print(f"{name:<12}: {value:.6f}")


def improvement_percent(baseline, improved):
    if baseline == 0:
        return 0.0
    return (baseline - improved) / baseline * 100.0



# ============================================================
# 3. DOC DU LIEU TRAIN VA TEST
# ============================================================

print("=" * 78)
print("LINEAR REGRESSION + SVR-RBF")
print("TRAIN: Tongket.csv | TEST: solieudonongdo.csv")
print("=" * 78)

train_raw = read_csv_checked(TRAIN_FILE)
test_raw = read_csv_checked(TEST_FILE)

print("\nCot trong file train:", train_raw.columns.tolist())
print("Cot trong file test :", test_raw.columns.tolist())

# Tongket.csv co cac cot: GROUP, F, C
required_train = ["GROUP", "F", "C"]

missing_train = [
    col for col in required_train
    if col not in train_raw.columns
]

if missing_train:
    raise ValueError(
        f"Tongket.csv thieu cot: {missing_train}. "
        "Can cac cot GROUP, F, C."
    )

# solieudonongdo.csv co cac cot: C_thuc, F0, F, C_linear
required_test = ["C_thuc", "F0", "F"]

missing_test = [
    col for col in required_test
    if col not in test_raw.columns
]

if missing_test:
    raise ValueError(
        f"solieudonongdo.csv thieu cot: {missing_test}. "
        "Can cac cot C_thuc, F0, F."
    )

# Lay dung cac cot can thiet
train = train_raw[["GROUP", "F", "C"]].copy()
test = test_raw.copy()

# Chuyen cac cot train sang dang so
for col in ["GROUP", "F", "C"]:
    train[col] = pd.to_numeric(
        train[col], errors="coerce"
    )

# Chuyen cac cot test sang dang so
for col in required_test:
    test[col] = pd.to_numeric(
        test[col], errors="coerce"
    )

# Loai bo gia tri khong hop le
train = train.replace(
    [np.inf, -np.inf], np.nan
)

test = test.replace(
    [np.inf, -np.inf], np.nan
)

train = train.dropna(
    subset=["GROUP", "F", "C"]
).copy()

test = test.dropna(
    subset=required_test
).copy()

# Tan so phai duong, nong do khong am
train = train[
    (train["F"] > 0) &
    (train["C"] >= 0)
].copy()

test = test[
    (test["F0"] > 0) &
    (test["F"] > 0)
].copy()

if train.empty:
    raise ValueError(
        "Khong con mau train hop le trong Tongket.csv."
    )

if test.empty:
    raise ValueError(
        "Khong con mau test hop le trong solieudonongdo.csv."
    )

print("\nSo mau train hop le:", len(train))
print("So mau test hop le :", len(test))

# ============================================================
# 4. TINH F0 THEO TUNG GROUP TREN TAP TRAIN
# ============================================================

# F0 cua moi GROUP la gia tri F trung binh tai C = 0.
blank_frequency = (
    train.loc[train["C"] == 0]
    .groupby("GROUP")["F"]
    .mean()
)

if blank_frequency.empty:
    raise ValueError(
        "Khong tim thay mau C = 0 trong Tongket.csv de tinh F0."
    )

train["F0"] = train["GROUP"].map(blank_frequency)
train = train.dropna(subset=["F0"]).copy()
train = train[(train["F0"] > 0) & (train["F"] > 0)].copy()

if train["GROUP"].nunique() < N_SPLITS:
    raise ValueError(
        f"Chi co {train['GROUP'].nunique()} GROUP hop le, "
        f"khong du cho GroupKFold {N_SPLITS} fold. "
        "Kiem tra cac GROUP co mau C=0."
    )

# A = log10(F0/F)
train["A"] = np.log10(train["F0"] / train["F"])

# Nong do tu duong chuan Linear Regression
train["C_LR"] = (train["A"] - B0) / B1

# SVR hoc sai so so voi Linear Regression
train["Residual"] = train["C"] - train["C_LR"]

# Loai bo truong hop khong hop le neu co
train = train.replace([np.inf, -np.inf], np.nan)
train = train.dropna(subset=["A", "C_LR", "Residual"]).copy()

print("\nSo GROUP train:", train["GROUP"].nunique())
print("So GROUP co mau blank:", len(blank_frequency))
print("So mau train cuoi cung:", len(train))


# ============================================================
# 5. TAO MO HINH SVR-RBF
# ============================================================

X_train = train[["A", "C_LR"]].to_numpy(dtype=float)
y_train = train["Residual"].to_numpy(dtype=float)
C_true_train = train["C"].to_numpy(dtype=float)
C_lr_train = train["C_LR"].to_numpy(dtype=float)

if len(train) < N_SPLITS:
    raise ValueError(
        f"Can it nhat {N_SPLITS} mau train de chay Cross Validation."
    )

svr_rbf = make_pipeline(
    StandardScaler(),
    SVR(
        kernel="rbf",
        C=SVR_C,
        gamma="scale",
        epsilon=SVR_EPSILON
    )
)


# ============================================================
# 6. CROSS VALIDATION TREN TAP TRAIN
# ============================================================

# KFold danh gia tren cac mau train.
# Luu y: cac mau cung GROUP co the nam o nhieu fold khac nhau.
kf = KFold(
    n_splits=N_SPLITS,
    shuffle=True,
    random_state=RANDOM_STATE
)

print("\nDang chay Cross Validation tren Tongket.csv...")
residual_cv = cross_val_predict(
    svr_rbf,
    X_train,
    y_train,
    cv=kf
)

C_svr_cv = C_lr_train + residual_cv

metrics_lr_cv = calculate_metrics(C_true_train, C_lr_train)
metrics_svr_cv = calculate_metrics(C_true_train, C_svr_cv)

print_metrics("TRAIN CROSS VALIDATION - LINEAR REGRESSION", metrics_lr_cv)
print_metrics("TRAIN CROSS VALIDATION - LINEAR + SVR-RBF", metrics_svr_cv)


# ============================================================
# 7. HUAN LUYEN MODEL CUOI TREN TOAN BO TAP TRAIN
# ============================================================

print("\nDang huan luyen model cuoi tren toan bo Tongket.csv...")
svr_rbf.fit(X_train, y_train)

svr_model = svr_rbf.named_steps["svr"]
scaler = svr_rbf.named_steps["standardscaler"]

support_vectors = svr_model.support_vectors_
dual_coef = svr_model.dual_coef_[0]
intercept = float(svr_model.intercept_[0])
gamma = float(svr_model._gamma)
scaler_mean = scaler.mean_
scaler_scale = scaler.scale_
n_sv = len(support_vectors)

print("Da huan luyen xong.")
print("So Support Vector:", n_sv)
print(f"Gamma: {gamma:.12f}")
print(f"Intercept: {intercept:.12f}")


# ============================================================
# 8. KIEM THU TREN solieudonongdo.csv
# ============================================================

# Khong fit/train lai tren tap test.
# C_thuc chi duoc dung de danh gia sau khi du doan.
test["A"] = np.log10(test["F0"] / test["F"])
test["C_LR_calc"] = (test["A"] - B0) / B1

X_test = test[["A", "C_LR_calc"]].to_numpy(dtype=float)

test["Residual_SVR"] = svr_rbf.predict(X_test)
test["C_SVR"] = test["C_LR_calc"] + test["Residual_SVR"]

test["Error_LR"] = test["C_LR_calc"] - test["C_thuc"]
test["Error_SVR"] = test["C_SVR"] - test["C_thuc"]
test["Abs_Error_LR"] = test["Error_LR"].abs()
test["Abs_Error_SVR"] = test["Error_SVR"].abs()

metrics_lr_test = calculate_metrics(
    test["C_thuc"].to_numpy(),
    test["C_LR_calc"].to_numpy()
)
metrics_svr_test = calculate_metrics(
    test["C_thuc"].to_numpy(),
    test["C_SVR"].to_numpy()
)

print_metrics("TEST - LINEAR REGRESSION", metrics_lr_test)
print_metrics("TEST - LINEAR + SVR-RBF", metrics_svr_test)

print("\n" + "=" * 78)
print("MUC CAI THIEN TREN TAP TEST")
print("=" * 78)
print(
    f"Cai thien MAE : "
    f"{improvement_percent(metrics_lr_test['MAE'], metrics_svr_test['MAE']):.2f}%"
)
print(
    f"Cai thien RMSE: "
    f"{improvement_percent(metrics_lr_test['RMSE'], metrics_svr_test['RMSE']):.2f}%"
)


# ============================================================
# 9. IN TOAN BO KET QUA TEST
# ============================================================

show_columns = [
    "C_thuc",
    "F0",
    "F",
    "A",
    "C_LR_calc",
    "Residual_SVR",
    "C_SVR",
    "Error_LR",
    "Error_SVR",
    "Abs_Error_LR",
    "Abs_Error_SVR"
]

print("\n" + "=" * 110)
print("TOAN BO KET QUA KIEM THU")
print("=" * 110)

pd.set_option("display.max_rows", None)
pd.set_option("display.max_columns", None)
pd.set_option("display.width", None)

print(
    test[show_columns].to_string(
        index=False,
        float_format=lambda value: f"{value:.6f}"
    )
)
print("\nTong so mau test:", len(test))


# ============================================================
# 10. XUAT HEADER C CHO ATMEGA2560
# ============================================================

print("\nDang xuat", OUTPUT_H, "...")

with open(OUTPUT_H, "w", encoding="utf-8") as f:
    f.write(
        "#ifndef SVR_RBF_MODEL_H\n"
        "#define SVR_RBF_MODEL_H\n\n"
        "#include <stdint.h>\n"
        "#include <math.h>\n\n"
    )

    f.write("// Linear calibration: A = B1*C + B0\n")
    f.write(f"#define LR_B0 {B0:.12f}f\n")
    f.write(f"#define LR_B1 {B1:.12f}f\n\n")

    f.write("// SVR-RBF parameters\n")
    f.write(f"#define SVR_N_SUPPORT {n_sv}\n")
    f.write(f"#define SVR_GAMMA {gamma:.12f}f\n")
    f.write(f"#define SVR_INTERCEPT {intercept:.12f}f\n\n")

    f.write("static const float scaler_mean[2] = {\n")
    f.write(f"    {scaler_mean[0]:.12f}f,\n")
    f.write(f"    {scaler_mean[1]:.12f}f\n")
    f.write("};\n\n")

    f.write("static const float scaler_scale[2] = {\n")
    f.write(f"    {scaler_scale[0]:.12f}f,\n")
    f.write(f"    {scaler_scale[1]:.12f}f\n")
    f.write("};\n\n")

    # support_vectors_ cua sklearn SVR nam trong khong gian da scale,
    # vi SVR duoc dat sau StandardScaler trong pipeline.
    f.write(
        "static const float support_vectors[SVR_N_SUPPORT][2] = {\n"
    )
    for i, sv in enumerate(support_vectors):
        comma = "," if i < n_sv - 1 else ""
        f.write(
            f"    {{{sv[0]:.12f}f, {sv[1]:.12f}f}}{comma}\n"
        )
    f.write("};\n\n")

    f.write("static const float dual_coef[SVR_N_SUPPORT] = {\n")
    for i, alpha in enumerate(dual_coef):
        comma = "," if i < n_sv - 1 else ""
        f.write(f"    {alpha:.12f}f{comma}\n")
    f.write("};\n\n")

    f.write(
        r"""
/*
 * Du doan Residual bang SVR-RBF.
 * A va C_LR la gia tri chua scale.
 */
static float SVR_RBF_predict(float A, float C_LR)
{
    float x0 = (A - scaler_mean[0]) / scaler_scale[0];
    float x1 = (C_LR - scaler_mean[1]) / scaler_scale[1];
    float residual = 0.0f;

    for (uint16_t i = 0; i < SVR_N_SUPPORT; i++) {
        float dx0 = x0 - support_vectors[i][0];
        float dx1 = x1 - support_vectors[i][1];
        float distance = dx0 * dx0 + dx1 * dx1;
        residual += dual_coef[i] * expf(-SVR_GAMMA * distance);
    }

    return residual + SVR_INTERCEPT;
}

/*
 * Tinh nong do H2O2:
 * A = log10(F0/F)
 * C_LR = (A - LR_B0)/LR_B1
 * C_SVR = C_LR + Residual
 *
 * Tra ve -1 neu F0 hoac F khong hop le.
 */
static float calculate_H2O2_SVR(float F0, float F)
{
    if (F0 <= 0.0f || F <= 0.0f) {
        return -1.0f;
    }

    float A = log10f(F0 / F);
    float C_LR = (A - LR_B0) / LR_B1;
    float residual = SVR_RBF_predict(A, C_LR);

    return C_LR + residual;
}

#endif
"""
    )


# ============================================================
# 11. XUAT CSV KET QUA TEST
# ============================================================

test.to_csv(
    OUTPUT_CSV,
    index=False,
    encoding="utf-8-sig"
)


# ============================================================
# 12. XUAT BAO CAO THONG SO
# ============================================================

with open(OUTPUT_TXT, "w", encoding="utf-8") as f:
    f.write("LINEAR REGRESSION + SVR-RBF REPORT\n")
    f.write("=" * 70 + "\n\n")

    f.write(f"Training file: {TRAIN_FILE}\n")
    f.write(f"Testing file: {TEST_FILE}\n")
    f.write(f"Training samples: {len(train)}\n")
    f.write(f"Testing samples: {len(test)}\n")
    f.write(f"Training groups: {train['GROUP'].nunique()}\n")
    f.write(f"Support vectors: {n_sv}\n\n")

    f.write("Linear calibration:\n")
    f.write(f"B0 = {B0}\n")
    f.write(f"B1 = {B1}\n\n")

    f.write("SVR parameters:\n")
    f.write("Kernel = RBF\n")
    f.write(f"C = {SVR_C}\n")
    f.write(f"Epsilon = {SVR_EPSILON}\n")
    f.write(f"Gamma = {gamma}\n")
    f.write(f"Intercept = {intercept}\n")
    f.write(f"Scaler mean = {scaler_mean.tolist()}\n")
    f.write(f"Scaler scale = {scaler_scale.tolist()}\n\n")

    f.write("TRAIN cross-validation metrics:\n")
    for key, value in metrics_lr_cv.items():
        f.write(f"Linear {key} = {value:.10f}\n")
    for key, value in metrics_svr_cv.items():
        f.write(f"Linear+SVR {key} = {value:.10f}\n")

    f.write("\nTEST metrics:\n")
    for key, value in metrics_lr_test.items():
        f.write(f"Linear {key} = {value:.10f}\n")
    for key, value in metrics_svr_test.items():
        f.write(f"Linear+SVR {key} = {value:.10f}\n")

    f.write("\nTest improvement:\n")
    f.write(
        "MAE improvement (%) = "
        f"{improvement_percent(metrics_lr_test['MAE'], metrics_svr_test['MAE']):.6f}\n"
    )
    f.write(
        "RMSE improvement (%) = "
        f"{improvement_percent(metrics_lr_test['RMSE'], metrics_svr_test['RMSE']):.6f}\n"
    )


# ============================================================
# 13. KET THUC
# ============================================================

print("\n" + "=" * 78)
print("HOAN THANH")
print("=" * 78)
print("Da train mo hinh bang:", TRAIN_FILE)
print("Da kiem thu mo hinh bang:", TEST_FILE)
print("\nCac file da tao:")
print("1.", OUTPUT_CSV)
print("2.", OUTPUT_TXT)
print("3.", OUTPUT_H)
print("\nLuu y: solieudonongdo.csv khong duoc dung de train model.")
print("SVR_RBF_model.h duoc tao tu model fit tren toan bo Tongket.csv.")
