# Tweetoscope project : École CentraleSupelec 2021-2022 

![CentraleSupelec Logo](https://www.centralesupelec.fr/sites/all/themes/cs_theme/medias/common/images/intro/logo_nouveau.jpg)


## Authors 
* Matthieu Briet : matthieu.briet@student-cs.fr
* Tanguy Colleville : tanguy.colleville@student-cs.fr
* Antoine Pagneux : antoine.pagneux@student-cs.fr

## Useful links 
* Our Workspace : [Here](https://tanguycolleville.notion.site/Tweetoscope_2021_11-4ee9e24f4bf14f8aa0896e83d75d0862)
* Our video link : [Here](https://youtu.be/8RnYTY3E3Lc)
(Please note that our voice have changed because of video treatment)
* Report is in the wiki gitlab section

## Summary
  - [Authors ](#authors-)
  - [Useful links](#Useful-links)
  - [Summary](#summary)
  - [Introduction](#introduction)
  - [Architecture & overview](#architecture--overview)
  - [How to use](#how-to-use)
    - [Instruction for installation](#instruction-for-installation)
    - [Fault tolerance](#fault-tolerance)
    - [Scalability](#scalability)
  - [Conclusion](#conclusion)

## Introduction
Here you are to see the so called tweetoscope 3rd year project as part of MDS_SDI mention and aimed to predict tweet popularity. All of it is based on fundamentals knowledges from SAE, Advanced C++, ML and statistical models courses. We want to detect as soon as possible tweets that are likely to become popular, where popularity of a tweet is defined as the number of times this tweet will be retweeted, i.e. forwarded by users to their followers. Because retweets propagate with a cascade effect, a tweet and all retweets it triggered, build up what is hereafter called a cascade. Once we are able to predict popularities of tweets, it is straight-forward to identify the most promising tweets with the highest expected popularity. So to summarize, the problem is to guess the final size of cascades, just by observing the beginning of it during a given observation time window, like, say, the ten first minutes.


## Code documentation 
* You can access to python code documentation in `docs/_build/hmtl/index.html`
* You can access to python coverage report in `coverage/index.html`
* You can access to python pylint report in `Rapport_Pylint/report.txt`
* You can acces to c++ documentation in `collector/docs/html/index.html`

## Architecture & overview
Our architecture ensure stability and safety because it relies on a well done Kubernetes x Docker collaboration. Moreover we have used agile methods, like ci-cd gitlab features.
Tweetoscope's architecture : 
![architecture](https://pennerath.pages.centralesupelec.fr/tweetoscope/graphviz-images/ead74cb4077631acad74606a761525fe2a3228c1.svg)


Repo's architecture : 
```
.
├── collector
│   ├── docs
│   │   ├── html
│   │   └── latex
│   ├── Doxyfile
│   ├── params.config
│   ├── params-deploy.config
│   ├── tweet-collector
│   ├── tweet-collector.cpp
│   └── tweetoscopeCollectorParams.hpp
├── coverage
│   ├── coverage_html.js
│   ├── favicon_32.png
│   ├── index.html
│   ├── keybd_closed.png
│   ├── keybd_open.png
│   ├── status.json
│   └── style.css
├── Data
│   ├── CSV
│   │   ├── news-data.csv
│   │   └── news-index.csv
│   └── Index
│       ├── cascades.idx
│       └── tweets.idx
├── Docker_files
│   ├── DockerFile.Collector
│   ├── DockerFile.Estimator
│   ├── DockerFile.Gen_GAML
│   ├── DockerFile.Learner
│   ├── DockerFile.Predictor
│   └── DockerFile.Python
├── docs
│   ├── _build
│   │   ├── doctrees
│   │   └── html
│   ├── conf.py
│   ├── index.rst
│   ├── logo_cs.png
│   ├── make.bat
│   ├── Makefile
│   ├── modules.rst
│   └── Python_files.rst
├── generator
│   ├── params.config
│   ├── params-deploy.config
│   ├── tweet-generator
│   ├── tweet-generator.cpp
│   └── tweetoscopeGenerator.hpp
├── Python_files
│   ├── cascade_class.py
│   ├── hawkes_estimator.py
│   ├── hawkes_tools.py
│   ├── __init__.py
│   ├── learner.py
│   ├── logger.py
│   ├── predictor.py
│   ├── predictor_tools.py
│   └── predictor_v2.py
├── Rapport_Pylint
│   └── report.txt
├── README.md
├── requirements.txt
├── tests
│   ├── test_cascade.npy
│   └── test_hawkes.py
└── YAML_files
    ├── deployment_base_intercell.yml
    ├── deployment_base_minikube.yml
    ├── tweetoscope_intercell.yml
    └── tweetoscope_minikube.yml
```



## How to use 
### Instruction for installation
To set up and use our product you will have to do the following : 
```
git clone https://gitlab-student.centralesupelec.fr/tanguy.colleville/tweetoscope_2021_11.git
```

Minikube deployment


```
minikube start
git clone https://gitlab-student.centralesupelec.fr/tanguy.colleville/tweetoscope_2021_11.git
cd tweetoscope_2021_11/YAML_files
kubectl apply -f deployment_base.yml
kubectl apply -f tweetoscope_local.yml
```


Intercell deployment 

```
ssh cpusdi1_20@phome.metz.supelec.fr
ssh ic25
git clone https://gitlab-student.centralesupelec.fr/tanguy.colleville/tweetoscope_2021_11.git
cd tweetoscope_2021_11/YAML_files
kubectl -n cpusdi1-20-ns apply -f deployment_base_intercell.yml
kubectl -n cpusdi1-20-ns apply -f tweetoscope_intercell.yml
```

You can appreciate created pods by using 

```
kubectl -n cpusdi1-20-ns get pods -o wide
kubectl -n cpusdi1-20-ns describe  pod's_name
```

### Fault Tolerance 
You will be able to appreciate the fault tolerance through this command. Indeed you will notice that Kubernetes has recreated a pod on a new ip and a new node.

```
kubectl -n cpusdi1-20-ns delete pod nom_du_pod
kubectl -n cpusdi1-20-ns get pods -o wide
```

### Scalability 

You will be able to appreciate the scalability through this command. Indeed you will notice that Kubernetes has scaled the node as many as replicas we wanted.

```
kubectl -n cpusdi1-20-ns get deployments
kubectl -n cpusdi1-20-ns scale --replicas=3 deployment/nom_deployment
kubectl -n cpusdi1-20-ns get pods -o wide
```

## Conclusion
To conclude, we can say that even if this project was time consuming we have learn so much on good practice to get scalability, relybility, and continuous integration/delivery. 
