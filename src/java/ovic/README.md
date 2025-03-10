# OVIC Benchmarker for LPCV 2020

This folder contains the SDK for track one of the
[Low Power Computer Vision workshop at CVPR 2020.](https://lpcv.ai/2020CVPR/ovic-track)

## Pre-requisite

Follow the steps [here](https://www.tensorflow.org/lite/demo_android) to install
Tensorflow, Bazel, and the Android NDK and SDK.

## Test the benchmarker:

The testing utilities helps the developers (you) to make sure that your
submissions in TfLite format will be processed as expected in the competition's
benchmarking system.

Note: for now the tests only provides correctness checks, i.e. classifier
predicts the correct category on the test image, but no on-device latency
measurements. To test the latency measurement functionality, the tests will
print the latency running on a desktop computer, which is not indicative of the
on-device run-time. We are releasing an benchmarker Apk that would allow
developers to measure latency on their own devices.

### Obtain the sample models

The test data (models and images) should be downloaded automatically for you by
Bazel. In case they are not, you can manually install them as below.

Note: all commands should be called from your tensorflow installation folder
(under this folder you should find `tensorflow/lite`).

*   Download the
    [testdata package](https://storage.googleapis.com/download.tensorflow.org/data/ovic_2019_04_30.zip):

```sh
curl -L https://storage.googleapis.com/download.tensorflow.org/data/ovic_2019_04_30.zip -o /tmp/ovic.zip
```

*   Unzip the package into the testdata folder:

```sh
unzip -j /tmp/ovic.zip -d java/ovic/src/testdata/
```

### Run tests

You can run test with Bazel as below. This helps to ensure that the installation
is correct.

```sh
bazel test //java/ovic:OvicClassifierTest --cxxopt=-Wno-all --test_output=all

bazel test //java/ovic:OvicDetectorTest --cxxopt=-Wno-all --test_output=all
```

### Test your submissions

Once you have a submission that follows the instructions from the
[competition site](https://lpcv.ai/2020CVPR/ovic-track), you can verify it in
two ways:

#### Validate using randomly generated images

You can call the validator binary below to verify that your model fits the
format requirements. This often helps you to catch size mismatches (e.g. output
for classification should be [1, 1001] instead of [1,1,1,1001]). Let say the
submission file is located at `/path/to/my_model.lite`, then call:

```sh
bazel build //java/ovic:ovic_validator --cxxopt=-Wno-all
bazel-bin/java/ovic/ovic_validator /path/to/my_model.lite classify
```

Successful validation should print the following message to terminal:

```
Successfully validated /path/to/my_model.lite.

```

To validate detection models, use the same command but provide "detect" as the
second argument instead of "classify".

#### Test that the model produces sensible outcomes

You can go a step further to verify that the model produces results as expected.
This helps you catch bugs during TFLite conversion (e.g. using the wrong mean
and std values).

*   Move your submission to the testdata folder:

```sh
cp /path/to/my_model.lite java/ovic/src/testdata/
```

*   Resize the test image to the resolutions that are expected by your
    submission:

The test images can be found at
`java/ovic/src/testdata/test_image_*.jpg`. You may reuse these
images if your image resolutions are 128x128 or 224x224.

*   Add your model and test image to the BUILD rule at
    `java/ovic/src/testdata/BUILD`:

```JSON
filegroup(
    name = "ovic_testdata",
    srcs = [
        "@tflite_ovic_testdata//:detect.lite",
        "@tflite_ovic_testdata//:float_model.lite",
        "@tflite_ovic_testdata//:low_res_model.lite",
        "@tflite_ovic_testdata//:quantized_model.lite",
        "@tflite_ovic_testdata//:test_image_128.jpg",
        "@tflite_ovic_testdata//:test_image_224.jpg"
        "my_model.lite",        # <--- Your submission.
        "my_test_image.jpg",    # <--- Your test image.
    ],
    ...
```

*   For classification models, modify `OvicClassifierTest.java`:

    *   change `TEST_IMAGE_PATH` to `my_test_image.jpg`.

    *   change either `FLOAT_MODEL_PATH` or `QUANTIZED_MODEL_PATH` to
        `my_model.lite` depending on whether your model runs inference in float
        or
        [8-bit](https://github.com/tensorflow/tensorflow/tree/master/tensorflow/contrib/quantize).

    *   change `TEST_IMAGE_GROUNDTRUTH` (ImageNet class ID) to be consistent
        with your test image.

*   For detection models, modify `OvicDetectorTest.java`:

    *   change `TEST_IMAGE_PATH` to `my_test_image.jpg`.
    *   change `MODEL_PATH` to `my_model.lite`.
    *   change `GROUNDTRUTH` (COCO class ID) to be consistent with your test
        image.

Now you can run the bazel tests to catch any runtime issues with the submission.

Note: Please make sure that your submission passes the test. If a submission
fails to pass the test it will not be processed by the submission server.

## Measure on-device latency

We provide two ways to measure the on-device latency of your submission. The
first is through our competition server, which is reliable and repeatable, but
is limited to a few trials per day. The second is through the benchmarker Apk,
which requires a device and may not be as accurate as the server, but has a fast
turn-around and no access limitations. We recommend that the participants use
the benchmarker apk for early development, and reserve the competition server
for evaluating promising submissions.

### Running the benchmarker app

Make sure that you have followed instructions in
[Test your submissions](#test-your-submissions) to add your model to the
testdata folder and to the corresponding build rules.

Modify `java/ovic/demo/app/OvicBenchmarkerActivity.java`:

*   Add your model to the benchmarker apk by changing `modelPath` and
    `testImagePath` to your submission and test image.

```
  if (benchmarkClassification) {
    ...
    testImagePath = "my_test_image.jpg";
    modelPath = "my_model.lite";
  } else {  // Benchmarking detection.
  ...
```

If you are adding a detection model, simply modify `modelPath` and
`testImagePath` in the else block above.

*   Adjust the benchmark parameters when needed:

You can change the length of each experiment, and the processor affinity below.
`BIG_CORE_MASK` is an integer whose binary encoding represents the set of used
cores. This number is phone-specific. For example, Pixel 4 has 8 cores: the 4
little cores are represented by the 4 less significant bits, and the 4 big cores
by the 4 more significant bits. Therefore a mask value of 16, or in binary
`00010000`, represents using only the first big core. The mask 32, or in binary
`00100000` uses the second big core and should deliver identical results as the
mask 16 because the big cores are interchangeable.

```
  /** Wall time for each benchmarking experiment. */
  private static final double WALL_TIME = 3000;
  /** Maximum number of iterations in each benchmarking experiment. */
  private static final int MAX_ITERATIONS = 100;
  /** Mask for binding to a single big core. Pixel 1 (4), Pixel 4 (16). */
  private static final int BIG_CORE_MASK = 16;
```

Note: You'll need ROOT access to the phone to change processor affinity.

*   Build and install the app.

```
bazel build -c opt --cxxopt=-Wno-all //java/ovic/demo/app:ovic_benchmarker_binary
adb install -r bazel-bin/java/ovic/demo/app/ovic_benchmarker_binary.apk
```

Start the app and pick a task by clicking either the `CLF` button for
classification or the `DET` button for detection. The button should turn bright
green, signaling that the experiment is running. The benchmarking results will
be displayed after about the `WALL_TIME` you specified above. For example:

```
my_model.lite: Average latency=158.6ms after 20 runs.
```

### Sample latencies

Note: the benchmarking results can be quite different depending on the
background processes running on the phone. A few things that help stabilize the
app's readings are placing the phone on a cooling plate, restarting the phone,
and shutting down internet access.

Classification Model | Pixel 1 | Pixel 2 | Pixel 4
-------------------- | :-----: | ------: | :-----:
float_model.lite     | 97      | 113     | 37
quantized_model.lite | 73      | 61      | 13
low_res_model.lite   | 3       | 3       | 1

Detection Model        | Pixel 2 | Pixel 4
---------------------- | :-----: | :-----:
detect.lite            | 248     | 82
quantized_detect.lite  | 59      | 17
quantized_fpnlite.lite | 96      | 29

All latency numbers are in milliseconds. The Pixel 1 and Pixel 2 latency numbers
are measured on `Oct 17 2019` (Github commit hash
[I05def66f58fa8f2161522f318e00c1b520cf0606](https://github.com/tensorflow/tensorflow/commit/4b02bc0e0ff7a0bc02264bc87528253291b7c949#diff-4e94df4d2961961ba5f69bbd666e0552))

The Pixel 4 latency numbers are measured on `Apr 14 2020` (Github commit hash
[4b2cb67756009dda843c6b56a8b320c8a54373e0](https://github.com/tensorflow/tensorflow/commit/4b2cb67756009dda843c6b56a8b320c8a54373e0)).

Since Pixel 4 has excellent support for 8-bit quantized models, we strongly
recommend you to check out the
[Post-Training Quantization tutorial](https://www.tensorflow.org/lite/performance/post_training_quantization).

The detection models above are both single-shot models (i.e. no object proposal
generation) using TfLite's *fast* version of Non-Max-Suppression (NMS). The fast
NMS is significant faster than the regular NMS (used by the ObjectDetectionAPI
in training) at the expense of about 1% mAP for the listed models.

### Latency table

We have compiled a latency table for common neural network operators such as
convolutions, separable convolutions, and matrix multiplications. The table of
results is available here:

*   https://storage.cloud.google.com/ovic-data/

The results were generated by creating a small network containing a single
operation, and running the op under the test harness. For more details see the
NetAdapt paper<sup>1</sup>. We plan to expand table regularly as we test with
newer OS releases and updates to Tensorflow Lite.

### Sample benchmarks

Below are the baseline models (MobileNetV2, MnasNet, and MobileNetV3) used to
compute the reference accuracy for ImageNet classification. The naming
convention of the models are `[precision]_[model
class]_[resolution]_[multiplier]`. Pixel 2 Latency (ms) is measured on a single
Pixel 2 big core using the competition server on `Oct 17 2019`, while Pixel 4
latency (ms) is measured on a single Pixel 4 big core using the competition
server on `Apr 14 2020`. You can find these models on TFLite's
[hosted model page](https://www.tensorflow.org/lite/guide/hosted_models#image_classification).

Model                               | Pixel 2 | Pixel 4 | Top-1 Accuracy
:---------------------------------: | :-----: | :-----: | :------------:
quant_mobilenetv2_96_35             | 4       | 1       | 0.420
quant_mobilenetv2_96_50             | 5       | 1       | 0.478
quant_mobilenetv2_128_35            | 6       | 2       | 0.474
quant_mobilenetv2_128_50            | 8       | 2       | 0.546
quant_mobilenetv2_160_35            | 9       | 2       | 0.534
quant_mobilenetv2_96_75             | 8       | 2       | 0.560
quant_mobilenetv2_96_100            | 10      | 3       | 0.579
quant_mobilenetv2_160_50            | 12      | 3       | 0.583
quant_mobilenetv2_192_35            | 12      | 3       | 0.557
quant_mobilenetv2_128_75            | 13      | 3       | 0.611
quant_mobilenetv2_192_50            | 16      | 4       | 0.616
quant_mobilenetv2_128_100           | 16      | 4       | 0.629
quant_mobilenetv2_224_35            | 17      | 5       | 0.581
quant_mobilenetv2_160_75            | 20      | 5       | 0.646
float_mnasnet_96_100                | 21      | 7       | 0.625
quant_mobilenetv2_224_50            | 22      | 6       | 0.637
quant_mobilenetv2_160_100           | 25      | 6       | 0.674
quant_mobilenetv2_192_75            | 29      | 7       | 0.674
quant_mobilenetv2_192_100           | 35      | 9       | 0.695
float_mnasnet_224_50                | 35      | 12      | 0.679
quant_mobilenetv2_224_75            | 39      | 10      | 0.684
float_mnasnet_160_100               | 45      | 15      | 0.706
quant_mobilenetv2_224_100           | 48      | 12      | 0.704
float_mnasnet_224_75                | 55      | 18      | 0.718
float_mnasnet_192_100               | 62      | 20      | 0.724
float_mnasnet_224_100               | 84      | 27      | 0.742
float_mnasnet_224_130               | 126     | 40      | 0.758
float_v3-small-minimalistic_224_100 | -       | 5       | 0.620
quant_v3-small_224_100              | -       | 5       | 0.641
float_v3-small_224_75               | -       | 5       | 0.656
float_v3-small_224_100              | -       | 7       | 0.677
quant_v3-large_224_100              | -       | 12      | 0.728
float_v3-large_224_75               | -       | 15      | 0.735
float_v3-large-minimalistic_224_100 | -       | 17      | 0.722
float_v3-large_224_100              | -       | 20      | 0.753

### References

1.  **NetAdapt: Platform-Aware Neural Network Adaptation for Mobile
    Applications**<br />
    Yang, Tien-Ju, Andrew Howard, Bo Chen, Xiao Zhang, Alec Go, Mark Sandler,
    Vivienne Sze, and Hartwig Adam. In Proceedings of the European Conference
    on Computer Vision (ECCV), pp. 285-300. 2018<br />
    [[link]](https://arxiv.org/abs/1804.03230) arXiv:1804.03230, 2018.
