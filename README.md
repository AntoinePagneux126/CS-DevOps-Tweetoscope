# Tweetoscope project : École CentraleSupelec 2021-2022 

![CentraleSupelec Logo](https://www.centralesupelec.fr/sites/all/themes/cs_theme/medias/common/images/intro/logo_nouveau.jpg)


## Authors 
* Matthieu Briet : matthieu.briet@student-cs.fr
* Tanguy Colleville : tanguy.colleville@student-cs.fr
* Antoine Pagneux : antoine.pagneux@student-cs.fr

## Useful links 
* Our Workspace : [Here](https://tanguycolleville.notion.site/Tweetoscope_2021_11-4ee9e24f4bf14f8aa0896e83d75d0862)

## Summary
  - [Authors ](#authors-)
  - [Useful links](#Useful-links)
  - [Summary](#summary)
  - [Introduction](#introduction)
  - [Architecture & overview](#architecture--overview)
  - [How to use](#how-to-use)
    - [Instruction for installation](#instruction-for-installation)
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


## How to use 
To set up and use our product you will have to do the following : 
```
git clone https://gitlab-student.centralesupelec.fr/tanguy.colleville/tweetoscope_2021_11.git
```

Minikube deployment


```
minikube start
kubectl apply -f deployment_base.yml
kubectl apply -f tweetoscope_local.yml
```


Intercell deployment 

```
ssh cpusdi1_20@phome.metz.supelec.fr
ssh ic25
kubectl -n cpusdi1-20-ns apply -f deployment_base_intercell.yml
kubectl -n cpusdi1-20-ns apply -f tweetoscope_intercell.yml
```

You can appreciate created pods by using 

```
kubectl -n cpusdi1-20-ns get pods -o wide
kubectl -n cpusdi1-20-ns describe  pod's_name
```

## Conclusion
To conclude, we can say that even if this project was time consuming we have learn so much on good practice to get scalability, relybility, and continuous integration/delivery. 
