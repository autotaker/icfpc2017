train:
	# wget -O summary.json http://35.194.105.175:5000/summary/json
	python ./main.py train -A Flowlight -A Greedy2 -A KakeUdon -A Ichigo -A MCSlowLight -A Genocide

test:
	# wget -O summary.json http://35.194.105.175:5000/summary/json
	python ./main.py test \
		-M Sierpinski-triangle.json -M boston-sparse.json -M circle.json -M edinburgh-sparse.json -M gothenburg-sparse.json -M lambda.json -M nara-scaled.json -M nara-sparse.json -M oxford-3000-nodes.json -M oxford-center-sparse.json -M oxford-sparse.json -M oxford2-sparse-2.json -M randomMedium.json -M randomSparse.json -M sample.json -M tube.json -M van-city-sparse.json \
	-A Flowlight -A Greedy2 -A KakeUdon -A Ichigo -A MCSlowLight -A Genocide

eval:
	# wget -O summary.json http://35.194.105.175:5000/summary/json
	python ./main.py eval --test-maps lambda.json -A Flowlight -A Greedy2 -A KakeUdon -A Ichigo -A MCSlowLight -A Genocide
