import numpy as np
import pandas as pd
from scipy.stats import skew, kurtosis
from sklearn.neighbors import NearestNeighbors  # For kNN similarity

def extract_features(data):
    features = {}
    for axis in ["Acc_X", "Acc_Y", "Acc_Z"]:
        signal = data[axis]

        features[f"{axis}_mean"]      = np.mean(signal)
        features[f"{axis}_std"]       = np.std(signal)
        features[f"{axis}_min"]       = np.min(signal)
        features[f"{axis}_max"]       = np.max(signal)
        features[f"{axis}_zcr"]       = np.mean(np.diff(np.sign(signal)) != 0)
    return features

FOLDER = "samples"
FILE_PREFIX = "data"
NUM_FILES = 8

all_data = [pd.read_csv(f"{FOLDER}/{FILE_PREFIX}{i}.csv") for i in range(NUM_FILES)]
feature_list = [extract_features(df) for df in all_data]

feature_df = pd.DataFrame(feature_list)
print(feature_df)

# knn
nbrs = NearestNeighbors(n_neighbors=NUM_FILES, metric='euclidean')  # Euclidean distance
nbrs.fit(feature_df)


distances, indices = nbrs.kneighbors(feature_df)

for i in range(NUM_FILES):
    print(f"\nDataset {i} closest datasets:")
    for j, idx in enumerate(indices[i]):
        print(f"  Dataset {idx} at distance {distances[i][j]:.3f}")
