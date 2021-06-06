FROM python:3.10-rc-slim

ARG USER=docker
ARG UID=1000
ARG GID=1000

RUN apt-get update && apt-get install -y build-essential
RUN useradd -m ${USER} --uid=${UID} 

USER ${UID}:${GID}

WORKDIR /usr/src/app

COPY requirements.txt ./

RUN pip install --upgrade pip && pip install --no-cache-dir -r requirements.txt

CMD [ "echo", "./gen_qr_codes.py" ]
